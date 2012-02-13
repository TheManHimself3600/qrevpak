/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_main.c

#ifdef GXQUAKE

#include <gccore.h>

#include "quakedef.h"

extern Mtx44 gx_projection_matrix;

extern Mtx gx_modelview_matrices[32];

extern int gx_cur_modelview_matrix;

extern qboolean gx_cull_enabled;

extern u8 gx_cull_mode;

extern u8 gx_z_test_enabled;

extern u8 gx_z_write_enabled;

extern qboolean gx_blend_enabled;

extern u8 gx_blend_src_value;

extern u8 gx_blend_dst_value;

entity_t	r_worldentity;

qboolean	r_cache_thrash;		// compatability

vec3_t		modelorg, r_entorigin;
entity_t	*currententity;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

mplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

qboolean	envmap;				// true during envmap command capture 

int			currenttexture = -1;		// to avoid unnecessary texture sets

int			cnttextures[2] = {-1, -1};     // cached

int			particletexture;	// little dot for particles
int			playertextures;		// up to 16 color translated skins

int			mirrortexturenum;	// quake texturenum, not gxtexturenum
qboolean	mirror;
mplane_t	*mirror_plane;

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


void R_MarkLeaves (void);

cvar_t	r_norefresh = {"r_norefresh","0"};
cvar_t	r_drawentities = {"r_drawentities","1"};
cvar_t	r_drawviewmodel = {"r_drawviewmodel","1"};
cvar_t	r_speeds = {"r_speeds","0"};
cvar_t	r_fullbright = {"r_fullbright","0"};
cvar_t	r_lightmap = {"r_lightmap","0"};
cvar_t	r_shadows = {"r_shadows","0"};
cvar_t	r_mirroralpha = {"r_mirroralpha","1"};
cvar_t	r_wateralpha = {"r_wateralpha","1"};
cvar_t	r_dynamic = {"r_dynamic","1"};
cvar_t	r_novis = {"r_novis","0"};

cvar_t	gx_cull = {"gx_cull","1"};
cvar_t	gx_texsort = {"gx_texsort","1"};
cvar_t	gx_smoothmodels = {"gx_smoothmodels","1"};
cvar_t	gx_affinemodels = {"gx_affinemodels","0"};
cvar_t	gx_polyblend = {"gx_polyblend","1"};
cvar_t	gx_flashblend = {"gx_flashblend","1"};
cvar_t	gx_playermip = {"gx_playermip","0"};
cvar_t	gx_nocolors = {"gx_nocolors","0"};
cvar_t	gx_keeptjunctions = {"gx_keeptjunctions","0"};
cvar_t	gx_reporttjunctions = {"gx_reporttjunctions","0"};
cvar_t	gx_doubleeyes = {"gx_doubleeys", "1"};

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	for (i=0 ; i<4 ; i++)
		if (BoxOnPlaneSide (mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}

// The results of this rotation aren't applied immediately, since there might be further transformations pending...
void R_RotateForEntity (entity_t *e)
{
	Mtx m;
	guVector a;

	guMtxTrans(m, e->origin[0],  e->origin[1],  e->origin[2]);
	guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);
	a.x = 0;
	a.y = 0;
	a.z = 1;
	guMtxRotAxisDeg(m, &a, e->angles[1]);
	guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);
	a.y = 1;
	a.z = 0;
	guMtxRotAxisDeg(m, &a, -e->angles[0]);
	guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);
	a.x = 1;
	a.y = 0;
	guMtxRotAxisDeg(m, &a, e->angles[2]);
	guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *currententity)
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = currententity->model->cache.data;
	frame = currententity->frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_Printf ("R_DrawSprite: no such frame %d\n", frame);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + currententity->syncbase;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	vec3_t	point;
	mspriteframe_t	*frame;
	float		*up, *right;
	vec3_t		v_forward, v_right, v_up;
	msprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache
	frame = R_GetSpriteFrame (e);
	psprite = currententity->model->cache.data;

	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
		AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
	{	// normal sprite
		up = vup;
		right = vright;
	}

	glColor3f (1,1,1);

	GX_DisableMultitexture();

    GX_Bind(frame->gx_texturenum);

	glEnable (GL_ALPHA_TEST);
	glBegin (GL_QUADS);

	glTexCoord2f (0, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->left, right, point);
	glVertex3fv (point);

	glTexCoord2f (0, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->left, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->right, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->right, right, point);
	glVertex3fv (point);
	
	glEnd ();

	glDisable (GL_ALPHA_TEST);
}

/*
=============================================================

  ALIAS MODELS

=============================================================
*/


#define NUMVERTEXNORMALS	162

float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

vec3_t	shadevector;
float	shadelight, ambientlight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anorm_dots.h"
;

float	*shadedots = r_avertexnormal_dots[0];

int	lastposenum;

/*
=============
GX_DrawAliasFrame
=============
*/
void GX_DrawAliasFrame (aliashdr_t *paliashdr, int posenum)
{
	float	s, t;
	float 	l;
	int		i, j;
	int		index;
	trivertx_t	*v, *verts;
	int		list;
	int		*order;
	vec3_t	point;
	float	*normal;
	int		count;

lastposenum = posenum;

	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
			glBegin (GL_TRIANGLE_STRIP);

		do
		{
			// texture coordinates come from the draw list
			glTexCoord2f (((float *)order)[0], ((float *)order)[1]);
			order += 2;

			// normals and vertexes come from the frame list
			l = shadedots[verts->lightnormalindex] * shadelight;
			glColor3f (l, l, l);
			glVertex3f (verts->v[0], verts->v[1], verts->v[2]);
			verts++;
		} while (--count);

		glEnd ();
	}
}


/*
=============
GX_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void GX_DrawAliasShadow (aliashdr_t *paliashdr, int posenum)
{
	float	s, t, l;
	int		i, j;
	int		index;
	trivertx_t	*v, *verts;
	int		list;
	int		*order;
	vec3_t	point;
	float	*normal;
	float	height, lheight;
	int		count;

	lheight = currententity->origin[2] - lightspot[2];

	height = 0;
	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	height = -lheight + 1.0;

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
			glBegin (GL_TRIANGLE_STRIP);

		do
		{
			// texture coordinates come from the draw list
			// (skipped for shadows) glTexCoord2fv ((float *)order);
			order += 2;

			// normals and vertexes come from the frame list
			point[0] = verts->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point[1] = verts->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point[2] = verts->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;
//			height -= 0.001;
			glVertex3fv (point);

			verts++;
		} while (--count);

		glEnd ();
	}	
}



/*
=================
R_SetupAliasFrame

=================
*/
void R_SetupAliasFrame (int frame, aliashdr_t *paliashdr)
{
	int				pose, numposes;
	float			interval;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	pose = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		interval = paliashdr->frames[frame].interval;
		pose += (int)(cl.time / interval) % numposes;
	}

	GX_DrawAliasFrame (paliashdr, pose);
}



/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (entity_t *e)
{
	int			i, j;
	int			lnum;
	vec3_t		dist;
	float		add;
	model_t		*clmodel;
	vec3_t		mins, maxs;
	aliashdr_t	*paliashdr;
	trivertx_t	*verts, *v;
	int			index;
	float		s, t, an;
	int			anim;
	Mtx			m;

	clmodel = currententity->model;

	VectorAdd (currententity->origin, clmodel->mins, mins);
	VectorAdd (currententity->origin, clmodel->maxs, maxs);

	if (R_CullBox (mins, maxs))
		return;


	VectorCopy (currententity->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	//
	// get lighting information
	//

	ambientlight = shadelight = R_LightPoint (currententity->origin);

	// allways give the gun some light
	if (e == &cl.viewent && ambientlight < 24)
		ambientlight = shadelight = 24;

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if (cl_dlights[lnum].die >= cl.time)
		{
			VectorSubtract (currententity->origin,
							cl_dlights[lnum].origin,
							dist);
			add = cl_dlights[lnum].radius - Length(dist);

			if (add > 0) {
				ambientlight += add;
				//ZOID models should be affected by dlights as well
				shadelight += add;
			}
		}
	}

	// clamp lighting so it doesn't overbright as much
	if (ambientlight > 128)
		ambientlight = 128;
	if (ambientlight + shadelight > 192)
		shadelight = 192 - ambientlight;

	// ZOID: never allow players to go totally black
	i = currententity - cl_entities;
	if (i >= 1 && i<=cl.maxclients /* && !strcmp (currententity->model->name, "progs/player.mdl") */)
		if (ambientlight < 8)
			ambientlight = shadelight = 8;

	// HACK HACK HACK -- no fullbright colors, so make torches full light
	if (!strcmp (clmodel->name, "progs/flame2.mdl")
		|| !strcmp (clmodel->name, "progs/flame.mdl") )
		ambientlight = shadelight = 256;

	shadedots = r_avertexnormal_dots[((int)(e->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
	shadelight = shadelight / 200.0;
	
	an = e->angles[1]/180*M_PI;
	shadevector[0] = cos(-an);
	shadevector[1] = sin(-an);
	shadevector[2] = 1;
	VectorNormalize (shadevector);

	//
	// locate the proper data
	//
	paliashdr = (aliashdr_t *)Mod_Extradata (currententity->model);

	c_alias_polys += paliashdr->numtris;

	//
	// draw all the triangles
	//

	GX_DisableMultitexture();

	guMtxCopy(gx_modelview_matrices[gx_cur_modelview_matrix], gx_modelview_matrices[gx_cur_modelview_matrix + 1]);
	gx_cur_modelview_matrix++;

	R_RotateForEntity (e);

	if (!strcmp (clmodel->name, "progs/eyes.mdl") && gx_doubleeyes.value) {

		guMtxTrans(m, paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2] - (22 + 8));
		guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);

// double size of eyes, since they are really hard to see while hardware rendering
		guMtxScale(m, paliashdr->scale[0]*2, paliashdr->scale[1]*2, paliashdr->scale[2]*2);
		guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);
	} else {
		guMtxTrans(m, paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
		guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);
		guMtxScale(m, paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);
		guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);
	}

	GX_LoadPosMtxImm(gx_modelview_matrices[gx_cur_modelview_matrix], GX_PNMTX0);

	anim = (int)(cl.time*10) & 3;
    GX_Bind(paliashdr->gx_texturenum[currententity->skinnum][anim]);

	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (currententity->colormap != vid.colormap && !gx_nocolors.value)
	{
		i = currententity - cl_entities;
		if (i >= 1 && i<=cl.maxclients /* && !strcmp (currententity->model->name, "progs/player.mdl") */)
		    GX_Bind(playertextures - 1 + i);
	}

	if (gx_smoothmodels.value)
		glShadeModel (GL_SMOOTH);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (gx_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	R_SetupAliasFrame (currententity->frame, paliashdr);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glShadeModel (GL_FLAT);
	if (gx_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	gx_cur_modelview_matrix--;
	GX_LoadPosMtxImm(gx_modelview_matrices[gx_cur_modelview_matrix], GX_PNMTX0);

	if (r_shadows.value)
	{
		guMtxCopy(gx_modelview_matrices[gx_cur_modelview_matrix], gx_modelview_matrices[gx_cur_modelview_matrix + 1]);
		gx_cur_modelview_matrix++;
		R_RotateForEntity (e);
		GX_LoadPosMtxImm(gx_modelview_matrices[gx_cur_modelview_matrix], GX_PNMTX0);
		glDisable (GL_TEXTURE_2D);
		gx_blend_enabled = true;
		GX_SetBlendMode(GX_BM_BLEND, gx_blend_src_value, gx_blend_dst_value, GX_LO_NOOP); 
		glColor4f (0,0,0,0.5);
		GX_DrawAliasShadow (paliashdr, lastposenum);
		glEnable (GL_TEXTURE_2D);
		gx_blend_enabled = false;
		GX_SetBlendMode(GX_BM_NONE, gx_blend_src_value, gx_blend_dst_value, GX_LO_NOOP); 
		glColor4f (1,1,1,1);
		gx_cur_modelview_matrix--;
		GX_LoadPosMtxImm(gx_modelview_matrices[gx_cur_modelview_matrix], GX_PNMTX0);
	}

}

//==================================================================================

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities.value)
		return;

	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		switch (currententity->model->type)
		{
		case mod_alias:
			R_DrawAliasModel (currententity);
			break;

		case mod_brush:
			R_DrawBrushModel (currententity);
			break;

		default:
			break;
		}
	}

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		switch (currententity->model->type)
		{
		case mod_sprite:
			R_DrawSpriteModel (currententity);
			break;
		}
	}
}

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
	float		ambient[4], diffuse[4];
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add;
	dlight_t	*dl;
	int			ambientlight, shadelight;

	if (!r_drawviewmodel.value)
		return;

	if (chase_active.value)
		return;

	if (envmap)
		return;

	if (!r_drawentities.value)
		return;

	if (cl.items & IT_INVISIBILITY)
		return;

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	currententity = &cl.viewent;
	if (!currententity->model)
		return;

	j = R_LightPoint (currententity->origin);

	if (j < 24)
		j = 24;		// allways give some light on gun
	ambientlight = j;
	shadelight = j;

// add dynamic lights		
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		dl = &cl_dlights[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtract (currententity->origin, dl->origin, dist);
		add = dl->radius - Length(dist);
		if (add > 0)
			ambientlight += add;
	}

	ambient[0] = ambient[1] = ambient[2] = ambient[3] = (float)ambientlight / 128;
	diffuse[0] = diffuse[1] = diffuse[2] = diffuse[3] = (float)shadelight / 128;

	R_DrawAliasModel (currententity);
}


/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	guVector a;
	Mtx m;

	if (!gx_polyblend.value)
		return;
	if (!v_blend[3])
		return;

	GX_DisableMultitexture();

	glDisable (GL_ALPHA_TEST);
	gx_blend_enabled = true;
	GX_SetBlendMode(GX_BM_BLEND, gx_blend_src_value, gx_blend_dst_value, GX_LO_NOOP); 
	gx_z_test_enabled = GX_FALSE;
	GX_SetZMode(gx_z_test_enabled, GX_LEQUAL, gx_z_write_enabled);
	glDisable (GL_TEXTURE_2D);

	a.x = 1;
	a.y = 0;
	a.z = 0;
	guMtxRotAxisDeg(gx_modelview_matrices[gx_cur_modelview_matrix], &a, -90); // put Z going up
	a.x = 0;
	a.z = 1;
	guMtxRotAxisDeg(m, &a, 90); 
	guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]); // put Z going up

	GX_LoadPosMtxImm(gx_modelview_matrices[gx_cur_modelview_matrix], GX_PNMTX0);

	glColor4fv (v_blend);

	glBegin (GL_QUADS);

	glVertex3f (10, 100, 100);
	glVertex3f (10, -100, 100);
	glVertex3f (10, -100, -100);
	glVertex3f (10, 100, -100);
	glEnd ();

	gx_blend_enabled = false;
	GX_SetBlendMode(GX_BM_NONE, gx_blend_src_value, gx_blend_dst_value, GX_LO_NOOP); 
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_ALPHA_TEST);
}


int SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

	if (r_refdef.fov_x == 90) 
	{
		// front side is visible

		VectorAdd (vpn, vright, frustum[0].normal);
		VectorSubtract (vpn, vright, frustum[1].normal);

		VectorAdd (vpn, vup, frustum[2].normal);
		VectorSubtract (vpn, vup, frustum[3].normal);
	}
	else
	{
		// rotate VPN right by FOV_X/2 degrees
		RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_refdef.fov_x / 2 ) );
		// rotate VPN left by FOV_X/2 degrees
		RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_refdef.fov_x / 2 );
		// rotate VPN up by FOV_X/2 degrees
		RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_refdef.fov_y / 2 );
		// rotate VPN down by FOV_X/2 degrees
		RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_refdef.fov_y / 2 ) );
	}

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}



/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int				edgecount;
	vrect_t			vrect;
	float			w, h;

// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
		Cvar_Set ("r_fullbright", "0");

	R_AnimateLight ();

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	r_cache_thrash = false;

	c_brush_polys = 0;
	c_alias_polys = 0;

}


/*
=============
R_SetupGX
=============
*/
void R_SetupGX (void)
{
	float		screenaspect;
	float		yfov;
	int			i;
	extern	int gxwidth, gxheight;
	int			x, x2, y2, y, w, h;
	f32			xmin, xmax, ymin, ymax;
	Mtx44		m;
	guVector	a;

	//
	// set up viewpoint
	//
	x = r_refdef.vrect.x * gxwidth/vid.width;
	x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * gxwidth/vid.width;
	y = (vid.height-r_refdef.vrect.y) * gxheight/vid.height;
	y2 = (vid.height - (r_refdef.vrect.y + r_refdef.vrect.height)) * gxheight/vid.height;

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < gxwidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < gxheight)
		y++;

	w = x2 - x;
	h = y - y2;

	if (envmap)
	{
		x = y2 = 0;
		w = h = 256;
	}

	GX_SetViewport(gxx + x, gxy + y2, w, h, 0, 1);
	
	screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;
//	yfov = 2*atan((float)r_refdef.vrect.height/r_refdef.vrect.width)*180/M_PI;
	ymax = 4 * tan( r_refdef.fov_y * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * screenaspect;
	xmax = ymax * screenaspect;

	guFrustum(gx_projection_matrix, ymax, ymin, xmin, xmax, 4, 4096 );

	if (mirror)
	{
		if (mirror_plane->normal[2])
			guMtxScale(m, 1, -1, 1);
		else
			guMtxScale(m, -1, 1, 1);

		guMtxConcat(gx_projection_matrix, m, gx_projection_matrix);
		gx_cull_mode = GX_CULL_FRONT;
	}
	else
		gx_cull_mode = GX_CULL_BACK;

	if(gx_cull_enabled)
	{
		GX_SetCullMode(gx_cull_mode);
	};

	GX_LoadProjectionMtx(gx_projection_matrix, GX_PERSPECTIVE);

	a.x = 1;
	a.y = 0;
	a.z = 0;
	guMtxRotAxisDeg(gx_modelview_matrices[gx_cur_modelview_matrix], &a, -90);
	a.x = 0;
	a.z = 1;
	guMtxRotAxisDeg(m, &a, 90);
	guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]); // put Z going up
	a.x = 1;
	a.z = 0;
	guMtxRotAxisDeg(m, &a, -r_refdef.viewangles[2]);
	guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);
	a.x = 0;
	a.y = 1;
	guMtxRotAxisDeg(m, &a, -r_refdef.viewangles[0]);
	guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);
	a.y = 0;
	a.z = 1;
	guMtxRotAxisDeg(m, &a, -r_refdef.viewangles[1]);
	guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);
	guMtxTrans(m, -r_refdef.vieworg[0], -r_refdef.vieworg[1], -r_refdef.vieworg[2]);
	guMtxConcat(gx_modelview_matrices[gx_cur_modelview_matrix], m, gx_modelview_matrices[gx_cur_modelview_matrix]);

	GX_LoadPosMtxImm(gx_modelview_matrices[gx_cur_modelview_matrix], GX_PNMTX0);

	r_world_matrix[0] = gx_modelview_matrices[gx_cur_modelview_matrix][0][0];
	r_world_matrix[1] = gx_modelview_matrices[gx_cur_modelview_matrix][0][1];
	r_world_matrix[2] = gx_modelview_matrices[gx_cur_modelview_matrix][0][2];
	r_world_matrix[3] = gx_modelview_matrices[gx_cur_modelview_matrix][0][3];
	r_world_matrix[4] = gx_modelview_matrices[gx_cur_modelview_matrix][1][0];
	r_world_matrix[5] = gx_modelview_matrices[gx_cur_modelview_matrix][1][1];
	r_world_matrix[6] = gx_modelview_matrices[gx_cur_modelview_matrix][1][2];
	r_world_matrix[7] = gx_modelview_matrices[gx_cur_modelview_matrix][1][3];
	r_world_matrix[8] = gx_modelview_matrices[gx_cur_modelview_matrix][2][0];
	r_world_matrix[9] = gx_modelview_matrices[gx_cur_modelview_matrix][2][1];
	r_world_matrix[10] = gx_modelview_matrices[gx_cur_modelview_matrix][2][2];
	r_world_matrix[11] = gx_modelview_matrices[gx_cur_modelview_matrix][2][3];
	r_world_matrix[12] = 0;
	r_world_matrix[13] = 0;
	r_world_matrix[14] = 0;
	r_world_matrix[15] = 1;

	//
	// set drawing parms
	//
	if (gx_cull.value)
	{
		gx_cull_enabled = true;
		GX_SetCullMode(gx_cull_mode);
	} else
	{
		gx_cull_enabled = false;
		GX_SetCullMode(GX_CULL_NONE);
	};

	gx_blend_enabled = false;
	GX_SetBlendMode(GX_BM_NONE, gx_blend_src_value, gx_blend_dst_value, GX_LO_NOOP); 
	glDisable(GL_ALPHA_TEST);
	gx_z_test_enabled = GX_TRUE;
	GX_SetZMode(gx_z_test_enabled, GX_LEQUAL, gx_z_write_enabled);
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene (void)
{
	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGX ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();		// adds static entities to the list

	S_ExtraUpdate ();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList ();

	GX_DisableMultitexture();

	R_RenderDlights ();

	R_DrawParticles ();

#ifdef GXTEST
	Test_Draw ();
#endif

}


/*
=============
R_Mirror
=============
*/
void R_Mirror (void)
{
	float		d;
	msurface_t	*s;
	entity_t	*ent;
	Mtx44		sproj;

	if (!mirror)
		return;

	memcpy (r_base_world_matrix, r_world_matrix, sizeof(r_base_world_matrix));

	d = DotProduct (r_refdef.vieworg, mirror_plane->normal) - mirror_plane->dist;
	VectorMA (r_refdef.vieworg, -2*d, mirror_plane->normal, r_refdef.vieworg);

	d = DotProduct (vpn, mirror_plane->normal);
	VectorMA (vpn, -2*d, mirror_plane->normal, vpn);

	r_refdef.viewangles[0] = -asin (vpn[2])/M_PI*180;
	r_refdef.viewangles[1] = atan2 (vpn[1], vpn[0])/M_PI*180;
	r_refdef.viewangles[2] = -r_refdef.viewangles[2];

	ent = &cl_entities[cl.viewentity];
	if (cl_numvisedicts < MAX_VISEDICTS)
	{
		cl_visedicts[cl_numvisedicts] = ent;
		cl_numvisedicts++;
	}

	R_RenderScene ();
	R_DrawWaterSurfaces ();

	// blend on top
	gx_blend_enabled = true;
	GX_SetBlendMode(GX_BM_BLEND, gx_blend_src_value, gx_blend_dst_value, GX_LO_NOOP); 
	if (mirror_plane->normal[2])
		guMtxScale(sproj, 1, -1, 1);
	else
		guMtxScale(sproj, -1, 1, 1);
	guMtxConcat(gx_projection_matrix, sproj, gx_projection_matrix);
	GX_LoadProjectionMtx(gx_projection_matrix, GX_PERSPECTIVE);

	gx_cull_mode = GX_CULL_BACK;
	if(gx_cull_enabled)
	{
		GX_SetCullMode(gx_cull_mode);
	};

	gx_modelview_matrices[gx_cur_modelview_matrix][0][0] = r_base_world_matrix[0];
	gx_modelview_matrices[gx_cur_modelview_matrix][0][1] = r_base_world_matrix[1];
	gx_modelview_matrices[gx_cur_modelview_matrix][0][2] = r_base_world_matrix[2];
	gx_modelview_matrices[gx_cur_modelview_matrix][0][3] = r_base_world_matrix[3];
	gx_modelview_matrices[gx_cur_modelview_matrix][1][0] = r_base_world_matrix[4];
	gx_modelview_matrices[gx_cur_modelview_matrix][1][1] = r_base_world_matrix[5];
	gx_modelview_matrices[gx_cur_modelview_matrix][1][2] = r_base_world_matrix[6];
	gx_modelview_matrices[gx_cur_modelview_matrix][1][3] = r_base_world_matrix[7];
	gx_modelview_matrices[gx_cur_modelview_matrix][2][0] = r_base_world_matrix[8];
	gx_modelview_matrices[gx_cur_modelview_matrix][2][1] = r_base_world_matrix[9];
	gx_modelview_matrices[gx_cur_modelview_matrix][2][2] = r_base_world_matrix[10];
	gx_modelview_matrices[gx_cur_modelview_matrix][2][3] = r_base_world_matrix[11];
	GX_LoadPosMtxImm(gx_modelview_matrices[gx_cur_modelview_matrix], GX_PNMTX0);

	glColor4f (1,1,1,r_mirroralpha.value);
	s = cl.worldmodel->textures[mirrortexturenum]->texturechain;
	for ( ; s ; s=s->texturechain)
		R_RenderBrushPoly (s);
	cl.worldmodel->textures[mirrortexturenum]->texturechain = NULL;
	gx_blend_enabled = false;
	GX_SetBlendMode(GX_BM_NONE, gx_blend_src_value, gx_blend_dst_value, GX_LO_NOOP); 
	glColor4f (1,1,1,1);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	double	time1, time2;
	GLfloat colors[4] = {(GLfloat) 0.0, (GLfloat) 0.0, (GLfloat) 1, (GLfloat) 0.20};

	if (r_norefresh.value)
		return;

	if (!r_worldentity.model || !cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds.value)
	{
		time1 = Sys_FloatTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	mirror = false;

	// render normal view

/***** Experimental silly looking fog ******
****** Use r_fullbright if you enable ******
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, colors);
	glFogf(GL_FOG_END, 512.0);
	glEnable(GL_FOG);
********************************************/

	R_RenderScene ();
	R_DrawViewModel ();
	R_DrawWaterSurfaces ();

//  More fog right here :)
//	glDisable(GL_FOG);
//  End of all fog code...

	// render mirror view
	R_Mirror ();

	R_PolyBlend ();

	if (r_speeds.value)
	{
		time2 = Sys_FloatTime ();
		Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys); 
	}
}

#endif
