/*
Copyright (C) 1997-2001 Id Software, Inc.

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
#include "gx_local.h"

extern int gx_tex_allocated;

extern GXRModeObj* sys_rmode;

void R_Clear (void);

viddef_t	vid;

refimport_t	ri;

int GX_TEXTURE0, GX_TEXTURE1;

model_t		*r_worldmodel;

gxstate_t  gx_state;

image_t		*r_notexture;		// use for bad textures
image_t		*r_particletexture;	// little dot for particles

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

int			c_brush_polys, c_alias_polys;

float		v_blend[4];			// final blending color

void GX_Strings_f( void );

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

Mtx		r_world_matrix;
Mtx		r_base_world_matrix;

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t	*gl_nosubimage;
cvar_t	*gl_allow_software;

cvar_t	*gl_vertex_arrays;

cvar_t	*gl_particle_min_size;
cvar_t	*gl_particle_max_size;
cvar_t	*gl_particle_size;
cvar_t	*gl_particle_att_a;
cvar_t	*gl_particle_att_b;
cvar_t	*gl_particle_att_c;

cvar_t	*gl_ext_swapinterval;
cvar_t	*gl_ext_palettedtexture;
cvar_t	*gl_ext_multitexture;
cvar_t	*gl_ext_pointparameters;
cvar_t	*gl_ext_compiled_vertex_array;

cvar_t	*gl_log;
cvar_t	*gl_bitdepth;
cvar_t	*gl_drawbuffer;
cvar_t  *gl_driver;
cvar_t	*gl_lightmap;
cvar_t	*gl_shadows;
cvar_t	*gl_mode;
cvar_t	*gl_dynamic;
cvar_t  *gl_monolightmap;
cvar_t	*gl_modulate;
cvar_t	*gl_nobind;
cvar_t	*gl_round_down;
cvar_t	*gl_picmip;
cvar_t	*gl_skymip;
cvar_t	*gl_showtris;
cvar_t	*gl_ztrick;
cvar_t	*gl_finish;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_polyblend;
cvar_t	*gl_flashblend;
cvar_t	*gl_playermip;
cvar_t  *gl_saturatelighting;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_texturealphamode;
cvar_t	*gl_texturesolidmode;
cvar_t	*gl_lockpvs;

cvar_t	*gl_3dlabs_broken;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
cvar_t	*vid_ref;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	if (r_nocull->value)
		return false;

	for (i=0 ; i<4 ; i++)
		if ( BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


void R_RotateForEntity (entity_t *e)
{
	Mtx m;
	guVector a;

	qguMtxTrans(m, e->origin[0],  e->origin[1],  e->origin[2]);
	qguMtxConcat(gxu_modelview_matrices[gxu_cur_modelview_matrix], m, gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	a.x = 0;
	a.y = 0;
	a.z = 1;
	qguMtxRotAxisDeg(m, &a, e->angles[1]);
	qguMtxConcat(gxu_modelview_matrices[gxu_cur_modelview_matrix], m, gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	a.y = 1;
	a.z = 0;
	qguMtxRotAxisDeg(m, &a, -e->angles[0]);
	qguMtxConcat(gxu_modelview_matrices[gxu_cur_modelview_matrix], m, gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	a.x = 1;
	a.y = 0;
	qguMtxRotAxisDeg(m, &a, -e->angles[2]);
	qguMtxConcat(gxu_modelview_matrices[gxu_cur_modelview_matrix], m, gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	qgxLoadPosMtxImm(gxu_modelview_matrices[gxu_cur_modelview_matrix], GX_PNMTX0);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	float alpha = 1.0F;
	vec3_t	point;
	dsprframe_t	*frame;
	float		*up, *right;
	dsprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite = (dsprite_t *)currentmodel->extradata;

#if 0
	if (e->frame < 0 || e->frame >= psprite->numframes)
	{
		ri.Con_Printf (PRINT_ALL, "no such sprite frame %i\n", e->frame);
		e->frame = 0;
	}
#endif
	e->frame %= psprite->numframes;

	frame = &psprite->frames[e->frame];

#if 0
	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
	vec3_t		v_forward, v_right, v_up;

	AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
#endif
	{	// normal sprite
		up = vup;
		right = vright;
	}

	if ( e->flags & RF_TRANSLUCENT )
		alpha = e->alpha;

	if ( alpha != 1.0F )
		qgxSetBlendMode(GX_BM_BLEND, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);

    GX_Bind(currentmodel->skins[e->frame]->texnum);

	GX_TexEnv( GX_MODULATE );

	if ( alpha == 1.0 )
		qgxSetAlphaCompare(GX_GEQUAL, gxu_alpha_test_lower, GX_AOP_AND, GX_LEQUAL, gxu_alpha_test_higher);
	else
		qgxSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);

	qgxBegin (GX_QUADS, gxu_cur_vertex_format, 4);

	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	qgxPosition3f32 (point[0], point[1], point[2]);
	qgxColor4u8(255, 255, 255, alpha * 255);
	qgxTexCoord2f32 (0, 1);

	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	qgxPosition3f32 (point[0], point[1], point[2]);
	qgxColor4u8(255, 255, 255, alpha * 255);
	qgxTexCoord2f32 (0, 0);

	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	qgxPosition3f32 (point[0], point[1], point[2]);
	qgxColor4u8(255, 255, 255, alpha * 255);
	qgxTexCoord2f32 (1, 0);

	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	qgxPosition3f32 (point[0], point[1], point[2]);
	qgxColor4u8(255, 255, 255, alpha * 255);
	qgxTexCoord2f32 (1, 1);
	
	qgxEnd ();

	qgxSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
	GX_TexEnv( GX_REPLACE );

	if ( alpha != 1.0F )
		qgxSetBlendMode(GX_BM_NONE, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
}

//==================================================================================

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel (void)
{
	vec3_t	shadelight;
	int		i;

	if ( currententity->flags & RF_FULLBRIGHT )
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	else
		R_LightPoint (currententity->origin, shadelight);

	qguMtxCopy(gxu_modelview_matrices[gxu_cur_modelview_matrix], gxu_modelview_matrices[gxu_cur_modelview_matrix + 1]);
	gxu_cur_modelview_matrix++;
	R_RotateForEntity (currententity);

	qgxDisableTexture();

	qgxBegin (GX_TRIANGLEFAN, GX_VTXFMT0, 6);
	qgxPosition3f32 (0, 0, -16);
	qgxColor4u8(shadelight[0]*255, shadelight[1]*255, shadelight[2]*255, 255);
	for (i=0 ; i<=4 ; i++)
	{
		qgxPosition3f32 (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
		qgxColor4u8(shadelight[0]*255, shadelight[1]*255, shadelight[2]*255, 255);
	};
	qgxEnd ();

	qgxBegin (GX_TRIANGLEFAN, GX_VTXFMT0, 6);
	qgxPosition3f32 (0, 0, 16);
	qgxColor4u8(shadelight[0]*255, shadelight[1]*255, shadelight[2]*255, 255);
	for (i=4 ; i>=0 ; i--)
	{
		qgxPosition3f32 (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
		qgxColor4u8(shadelight[0]*255, shadelight[1]*255, shadelight[2]*255, 255);
	};
	qgxEnd ();

	gxu_cur_modelview_matrix--;
	qgxLoadPosMtxImm(gxu_modelview_matrices[gxu_cur_modelview_matrix], GX_PNMTX0);
	qgxEnableTexture();
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities->value)
		return;

	// draw non-transparent first
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;
			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	gxu_z_write_enabled = GX_FALSE;		// no z writes
	qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}
	gxu_z_write_enabled = GX_TRUE;		// back to writing
	qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);

}

/*
** GX_DrawParticles
**
*/
void GX_DrawParticles( int num_particles, const particle_t particles[], const unsigned colortable[768] )
{
	const particle_t *p;
	int				i;
	vec3_t			up, right;
	float			scale;
	byte			color[4];

    GX_Bind(r_particletexture->texnum);
	gxu_z_write_enabled = GX_FALSE;		// no z buffering
	qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
	qgxSetBlendMode(GX_BM_BLEND, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
	GX_TexEnv( GX_MODULATE );
	qgxBegin (GX_TRIANGLES, gxu_cur_vertex_format, num_particles * 3);

	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);

	for ( p = particles, i=0 ; i < num_particles ; i++,p++)
	{
		// hack a scale up to keep particles from disapearing
		scale = ( p->origin[0] - r_origin[0] ) * vpn[0] + 
			    ( p->origin[1] - r_origin[1] ) * vpn[1] +
			    ( p->origin[2] - r_origin[2] ) * vpn[2];

		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * 0.004;

		*(int *)color = colortable[p->color];
		color[3] = p->alpha*255;

		qgxPosition3f32( p->origin[0], p->origin[1], p->origin[2] );
		qgxColor4u8(color[0], color[1], color[2], color[3]);
		qgxTexCoord2f32( 0.0625, 0.0625 );
		
		qgxPosition3f32( p->origin[0] + up[0]*scale, 
			             p->origin[1] + up[1]*scale, 
					     p->origin[2] + up[2]*scale);
		qgxColor4u8(color[0], color[1], color[2], color[3]);
		qgxTexCoord2f32( 1.0625, 0.0625 );

		qgxPosition3f32( p->origin[0] + right[0]*scale, 
			             p->origin[1] + right[1]*scale, 
					     p->origin[2] + right[2]*scale);
		qgxColor4u8(color[0], color[1], color[2], color[3]);
		qgxTexCoord2f32( 0.0625, 1.0625 );
	}

	qgxEnd ();
	qgxSetBlendMode(GX_BM_NONE, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
	gxu_cur_r = 255;
	gxu_cur_g = 255;
	gxu_cur_b = 255;
	gxu_cur_a = 255;
	gxu_z_write_enabled = GX_TRUE;		// back to normal Z buffering
	qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
	GX_TexEnv( GX_REPLACE );
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	if ( gl_ext_pointparameters->value && qgxSetPointSize )
	{
		int i;
		unsigned char color[4];
		const particle_t *p;

		gxu_z_write_enabled = GX_FALSE;
		qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
		qgxSetBlendMode(GX_BM_BLEND, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
		qgxDisableTexture();

		qgxSetPointSize( gl_particle_size->value * 16, GX_TO_ZERO );

		qgxBegin (GX_POINTS, GX_VTXFMT0, r_newrefdef.num_particles);
		for ( i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++ )
		{
			*(int *)color = d_8to24table[p->color];
			color[3] = p->alpha*255;

			qgxPosition3f32( p->origin[0], p->origin[1], p->origin[2] );
			qgxColor4u8(color[0], color[1], color[2], color[3]);
		}
		qgxEnd();

		qgxSetBlendMode(GX_BM_NONE, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
		gxu_cur_r = 255;
		gxu_cur_g = 255;
		gxu_cur_b = 255;
		gxu_cur_a = 255;
		gxu_z_write_enabled = GX_TRUE;
		qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
		qgxEnableTexture();

	}
	else
	{
		GX_DrawParticles( r_newrefdef.num_particles, r_newrefdef.particles, d_8to24table );
	}
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	Mtx m;
	guVector v;
	u8 r, g, b, a;

	if (!gl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	qgxSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
	qgxSetBlendMode(GX_BM_BLEND, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
	gxu_z_test_enabled = GX_FALSE;
	qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
	qgxDisableTexture();

	// FIXME: get rid of these
	v.x = 1;
	v.y = 0;
	v.z = 0;
	qguMtxRotAxisDeg(gxu_modelview_matrices[gxu_cur_modelview_matrix], &v, -90); // put Z going up
	v.x = 0;
	v.z = 1;
	qguMtxRotAxisDeg(m, &v, 90); // put Z going up
	qguMtxConcat(gxu_modelview_matrices[gxu_cur_modelview_matrix], m, gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	qgxLoadPosMtxImm(gxu_modelview_matrices[gxu_cur_modelview_matrix], GX_PNMTX0);

	r = v_blend[0] * 255.0;
	g = v_blend[1] * 255.0;
	b = v_blend[2] * 255.0;
	a = v_blend[3] * 255.0;

	qgxBegin (GX_QUADS, GX_VTXFMT0, 4);

	qgxPosition3f32 (10, 100, 100);
	qgxColor4u8(r, g, b, a);
	qgxPosition3f32 (10, -100, 100);
	qgxColor4u8(r, g, b, a);
	qgxPosition3f32 (10, -100, -100);
	qgxColor4u8(r, g, b, a);
	qgxPosition3f32 (10, 100, -100);
	qgxColor4u8(r, g, b, a);
	qgxEnd ();

	qgxSetBlendMode(GX_BM_NONE, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
	qgxEnableTexture();
	qgxSetAlphaCompare(GX_GEQUAL, gxu_alpha_test_lower, GX_AOP_AND, GX_LEQUAL, gxu_alpha_test_higher);
}

//=======================================================================

int SignbitsForPlane (cplane_t *out)
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

#if 0
	/*
	** this code is wrong, since it presume a 90 degree FOV both in the
	** horizontal and vertical plane
	*/
	// front side is visible
	VectorAdd (vpn, vright, frustum[0].normal);
	VectorSubtract (vpn, vright, frustum[1].normal);
	VectorAdd (vpn, vup, frustum[2].normal);
	VectorSubtract (vpn, vup, frustum[3].normal);

	// we theoretically don't need to normalize these vectors, but I do it
	// anyway so that debugging is a little easier
	VectorNormalize( frustum[0].normal );
	VectorNormalize( frustum[1].normal );
	VectorNormalize( frustum[2].normal );
	VectorNormalize( frustum[3].normal );
#else
	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_newrefdef.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_newrefdef.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_newrefdef.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_newrefdef.fov_y / 2 ) );
#endif

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int i;
	mleaf_t	*leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.vieworg, r_origin);

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);

// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i=0 ; i<4 ; i++)
		v_blend[i] = r_newrefdef.blend[i];

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
	{
		gxu_cur_projection_matrix++;
		qguOrtho(gxu_projection_matrices[gxu_cur_projection_matrix], 0, vid.height, 0, vid.width, GXU_ORTHO_ZNEAR, GXU_ORTHO_ZFAR);
		qgxLoadProjectionMtx(gxu_projection_matrices[gxu_cur_projection_matrix], GX_ORTHOGRAPHIC);
		qgxDisableTexture();
		qgxBegin (GX_QUADS, GX_VTXFMT0, 4);
		qgxPosition3f32 (r_newrefdef.x, r_newrefdef.y, GXU_ORTHO_ZCOORD_BOTTOM);
		qgxColor4u8 (77, 77, 77, 255);
		qgxPosition3f32 (r_newrefdef.x+r_newrefdef.width, r_newrefdef.y, GXU_ORTHO_ZCOORD_BOTTOM);
		qgxColor4u8 (77, 77, 77, 255);
		qgxPosition3f32 (r_newrefdef.x+r_newrefdef.width, r_newrefdef.y+r_newrefdef.height, GXU_ORTHO_ZCOORD_BOTTOM);
		qgxColor4u8 (77, 77, 77, 255);
		qgxPosition3f32 (r_newrefdef.x, r_newrefdef.y+r_newrefdef.height, GXU_ORTHO_ZCOORD_BOTTOM);
		qgxColor4u8 (77, 77, 77, 255);
		qgxEnd ();
		qgxEnableTexture();
		gxu_cur_projection_matrix--;
		qgxLoadProjectionMtx(gxu_projection_matrices[gxu_cur_projection_matrix], gxu_cur_projection_type);
	}
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
//	float	yfov;
	int		x, x2, y2, y, w, h;
	Mtx m;
	guVector a;

	//
	// set up viewport
	//
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(r_newrefdef.y * vid.height / vid.height);
	y2 = ceil((r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y2 - y;

	gxu_viewport_x = ((sys_rmode->viWidth - vid.width) / 2) + x;
	gxu_viewport_y = ((sys_rmode->viHeight - vid.height) / 2) + y;
	gxu_viewport_width = w;
	gxu_viewport_height = h;
	qgxSetViewport (gxu_viewport_x, gxu_viewport_y, gxu_viewport_width, gxu_viewport_height, gxu_depth_min, gxu_depth_max);

	//
	// set up projection matrix
	//
    screenaspect = (float)r_newrefdef.width/r_newrefdef.height;
//	yfov = 2*atan((float)r_newrefdef.height/r_newrefdef.width)*180/M_PI;
	qguPerspective(gxu_projection_matrices[gxu_cur_projection_matrix], r_newrefdef.fov_y,  screenaspect,  4,  4096);
	gxu_cur_projection_type = GX_PERSPECTIVE;
	qgxLoadProjectionMtx(gxu_projection_matrices[gxu_cur_projection_matrix], gxu_cur_projection_type);

	gxu_cull_mode = GX_CULL_BACK;
	if(gxu_cull_enabled)
		qgxSetCullMode(gxu_cull_mode);

	a.x = 1;
	a.y = 0;
	a.z = 0;
	qguMtxRotAxisDeg(gxu_modelview_matrices[gxu_cur_modelview_matrix], &a, -90); // put Z going up
	a.x = 0;
	a.z = 1;
	qguMtxRotAxisDeg(m, &a, 90); // put Z going up
	qguMtxConcat(gxu_modelview_matrices[gxu_cur_modelview_matrix], m, gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	a.x = 1;
	a.z = 0;
	qguMtxRotAxisDeg(m, &a, -r_newrefdef.viewangles[2]);
	qguMtxConcat(gxu_modelview_matrices[gxu_cur_modelview_matrix], m, gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	a.x = 0;
	a.y = 1;
	qguMtxRotAxisDeg(m, &a, -r_newrefdef.viewangles[0]);
	qguMtxConcat(gxu_modelview_matrices[gxu_cur_modelview_matrix], m, gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	a.y = 0;
	a.z = 1;
	qguMtxRotAxisDeg(m, &a, -r_newrefdef.viewangles[1]);
	qguMtxConcat(gxu_modelview_matrices[gxu_cur_modelview_matrix], m, gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	qguMtxTrans(m, -r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);
	qguMtxConcat(gxu_modelview_matrices[gxu_cur_modelview_matrix], m, gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	qgxLoadPosMtxImm(gxu_modelview_matrices[gxu_cur_modelview_matrix], GX_PNMTX0);

//	if ( gl_state.camera_separation != 0 && gl_state.stereo_enabled )
//		qglTranslatef ( gl_state.camera_separation, 0, 0 );

	qguMtxCopy(gxu_modelview_matrices[gxu_cur_modelview_matrix], r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull->value)
	{
		gxu_cull_enabled = true;
		qgxSetCullMode(gxu_cull_mode);
	}
	else
	{
		gxu_cull_enabled = false;
		qgxSetCullMode(GX_CULL_NONE);
	};

	qgxSetBlendMode(GX_BM_NONE, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
	qgxSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
	gxu_z_test_enabled = GX_TRUE;
	qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
}

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (gl_ztrick->value)
	{
		static int trickframe;

		gxu_clear_buffers = GX_FALSE;

		trickframe++;
		if (trickframe & 1)
		{
			gxu_depth_min = 0;
			gxu_depth_max = 0.49999;
			gxu_cur_z_func = GX_LEQUAL;
			qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
		}
		else
		{
			gxu_depth_min = 1;
			gxu_depth_max = 0.5;
			gxu_cur_z_func = GX_GEQUAL;
			qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
		}
	}
	else
	{
		if (gl_clear->value)
			gxu_clear_color_buffer = GX_TRUE;
		else
			gxu_clear_color_buffer = GX_FALSE;
		gxu_clear_buffers = GX_TRUE;
		gxu_depth_min = 0;
		gxu_depth_max = 1;
		gxu_cur_z_func = GX_LEQUAL;
		qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
	}

	qgxSetViewport (gxu_viewport_x, gxu_viewport_y, gxu_viewport_width, gxu_viewport_height, gxu_depth_min, gxu_depth_max);

}

void R_Flash( void )
{
	R_PolyBlend ();
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		ri.Sys_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlights ();

	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();

	R_DrawEntitiesOnList ();

	R_RenderDlights ();

	R_DrawParticles ();

	R_DrawAlphaSurfaces ();

	R_Flash();

	if (r_speeds->value)
	{
		ri.Con_Printf (PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps %9i bytes\n",
			c_brush_polys, 
			c_alias_polys, 
			c_visible_textures, 
			c_visible_lightmaps,
			gx_tex_allocated); 
	}
}

void	R_SetGX2D (void)
{
	// set 2D virtual screen size
	gxu_viewport_x = (sys_rmode->viWidth - vid.width) / 2;
	gxu_viewport_y = (sys_rmode->viHeight - vid.height) / 2;
	gxu_viewport_width = vid.width;
	gxu_viewport_height = vid.height;
	qgxSetViewport (gxu_viewport_x, gxu_viewport_y, gxu_viewport_width, gxu_viewport_height, gxu_depth_min, gxu_depth_max);
	qguOrtho(gxu_projection_matrices[gxu_cur_projection_matrix], 0, vid.height, 0, vid.width, GXU_ORTHO_ZNEAR, GXU_ORTHO_ZFAR);
	gxu_cur_projection_type = GX_ORTHOGRAPHIC;
	qgxLoadProjectionMtx(gxu_projection_matrices[gxu_cur_projection_matrix], gxu_cur_projection_type);
	qguMtxIdentity(gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	qgxLoadPosMtxImm(gxu_modelview_matrices[gxu_cur_modelview_matrix], GX_PNMTX0);
	gxu_z_test_enabled = GX_FALSE;
	qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
	gxu_cull_enabled = false;
	qgxSetCullMode(GX_CULL_NONE);
	qgxSetBlendMode(GX_BM_NONE, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
	qgxSetAlphaCompare(GX_GEQUAL, gxu_alpha_test_lower, GX_AOP_AND, GX_LEQUAL, gxu_alpha_test_higher);
	gxu_cur_r = 255;
	gxu_cur_g = 255;
	gxu_cur_b = 255;
	gxu_cur_a = 255;
}

/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint (r_newrefdef.vieworg, shadelight);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150*shadelight[0];
		else
			r_lightlevel->value = 150*shadelight[2];
	}
	else
	{
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150*shadelight[1];
		else
			r_lightlevel->value = 150*shadelight[2];
	}

}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame (refdef_t *fd)
{
	R_RenderView( fd );
	R_SetLightLevel ();
	R_SetGX2D ();
}


void R_Register( void )
{
	r_lefthand = ri.Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh = ri.Cvar_Get ("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get ("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get ("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get ("r_novis", "0", 0);
	r_nocull = ri.Cvar_Get ("r_nocull", "0", 0);
	r_lerpmodels = ri.Cvar_Get ("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get ("r_speeds", "0", 0);

	r_lightlevel = ri.Cvar_Get ("r_lightlevel", "0", 0);

	gl_nosubimage = ri.Cvar_Get( "gl_nosubimage", "0", 0 );
	gl_allow_software = ri.Cvar_Get( "gl_allow_software", "0", 0 );

	gl_particle_min_size = ri.Cvar_Get( "gl_particle_min_size", "2", CVAR_ARCHIVE );
	gl_particle_max_size = ri.Cvar_Get( "gl_particle_max_size", "40", CVAR_ARCHIVE );
	gl_particle_size = ri.Cvar_Get( "gl_particle_size", "40", CVAR_ARCHIVE );
	gl_particle_att_a = ri.Cvar_Get( "gl_particle_att_a", "0.01", CVAR_ARCHIVE );
	gl_particle_att_b = ri.Cvar_Get( "gl_particle_att_b", "0.0", CVAR_ARCHIVE );
	gl_particle_att_c = ri.Cvar_Get( "gl_particle_att_c", "0.01", CVAR_ARCHIVE );

	gl_modulate = ri.Cvar_Get ("gl_modulate", "1", CVAR_ARCHIVE );
	gl_log = ri.Cvar_Get( "gl_log", "0", 0 );
	gl_bitdepth = ri.Cvar_Get( "gl_bitdepth", "0", 0 );
	gl_mode = ri.Cvar_Get( "gl_mode", "0", CVAR_ARCHIVE ); // There is no mode "3" in GX
	gl_lightmap = ri.Cvar_Get ("gl_lightmap", "0", 0);
	gl_shadows = ri.Cvar_Get ("gl_shadows", "0", CVAR_ARCHIVE );
	gl_dynamic = ri.Cvar_Get ("gl_dynamic", "1", 0);
	gl_nobind = ri.Cvar_Get ("gl_nobind", "0", 0);
	gl_round_down = ri.Cvar_Get ("gl_round_down", "1", 0);
	gl_picmip = ri.Cvar_Get ("gl_picmip", "0", 0);
	gl_skymip = ri.Cvar_Get ("gl_skymip", "0", 0);
	gl_showtris = ri.Cvar_Get ("gl_showtris", "0", 0);
	gl_ztrick = ri.Cvar_Get ("gl_ztrick", "0", 0);
	gl_finish = ri.Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE);
	gl_clear = ri.Cvar_Get ("gl_clear", "0", 0);
	gl_cull = ri.Cvar_Get ("gl_cull", "1", 0);
	gl_polyblend = ri.Cvar_Get ("gl_polyblend", "1", 0);
	gl_flashblend = ri.Cvar_Get ("gl_flashblend", "0", 0);
	gl_playermip = ri.Cvar_Get ("gl_playermip", "0", 0);
	gl_monolightmap = ri.Cvar_Get( "gl_monolightmap", "0", 0 );
	gl_driver = ri.Cvar_Get( "gl_driver", "libogc-gx", CVAR_ARCHIVE );
	gl_texturemode = ri.Cvar_Get( "gl_texturemode", "GX_LIN_MIP_NEAR", CVAR_ARCHIVE );
	gl_texturealphamode = ri.Cvar_Get( "gl_texturealphamode", "default", CVAR_ARCHIVE );
	gl_texturesolidmode = ri.Cvar_Get( "gl_texturesolidmode", "default", CVAR_ARCHIVE );
	gl_lockpvs = ri.Cvar_Get( "gl_lockpvs", "0", 0 );

	gl_vertex_arrays = ri.Cvar_Get( "gl_vertex_arrays", "0", CVAR_ARCHIVE );

	gl_ext_swapinterval = ri.Cvar_Get( "gl_ext_swapinterval", "1", CVAR_ARCHIVE );
	gl_ext_palettedtexture = ri.Cvar_Get( "gl_ext_palettedtexture", "1", CVAR_ARCHIVE );
	gl_ext_multitexture = ri.Cvar_Get( "gl_ext_multitexture", "1", CVAR_ARCHIVE );
	gl_ext_pointparameters = ri.Cvar_Get( "gl_ext_pointparameters", "1", CVAR_ARCHIVE );
	gl_ext_compiled_vertex_array = ri.Cvar_Get( "gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE );

	gl_drawbuffer = ri.Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );
	gl_swapinterval = ri.Cvar_Get( "gl_swapinterval", "1", CVAR_ARCHIVE );

	gl_saturatelighting = ri.Cvar_Get( "gl_saturatelighting", "0", 0 );

	gl_3dlabs_broken = ri.Cvar_Get( "gl_3dlabs_broken", "1", CVAR_ARCHIVE );

	vid_fullscreen = ri.Cvar_Get( "vid_fullscreen", "0", CVAR_ARCHIVE );
	vid_gamma = ri.Cvar_Get( "vid_gamma", "1.0", CVAR_ARCHIVE );
	vid_ref = ri.Cvar_Get( "vid_ref", "soft", CVAR_ARCHIVE );

	ri.Cmd_AddCommand( "imagelist", GX_ImageList_f );
	ri.Cmd_AddCommand( "screenshot", GX_ScreenShot_f );
	ri.Cmd_AddCommand( "modellist", Mod_Modellist_f );
	ri.Cmd_AddCommand( "gl_strings", GX_Strings_f );
}

/*
==================
R_SetMode
==================
*/
qboolean R_SetMode (void)
{
	rserr_t err;
	qboolean fullscreen;

	if ( vid_fullscreen->modified )
	{
		ri.Con_Printf( PRINT_ALL, "R_SetMode() - CDS not allowed with this driver\n" );
		ri.Cvar_SetValue( "vid_fullscreen", !vid_fullscreen->value );
		vid_fullscreen->modified = false;
	}

	fullscreen = vid_fullscreen->value;

	vid_fullscreen->modified = false;
	gl_mode->modified = false;

	if ( ( err = GXimp_SetMode( &vid.width, &vid.height, gl_mode->value, fullscreen ) ) == rserr_ok )
	{
		gx_state.prev_mode = gl_mode->value;
	}
	else
	{
		if ( err == rserr_invalid_fullscreen )
		{
			ri.Cvar_SetValue( "vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			ri.Con_Printf( PRINT_ALL, "ref_gx::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GXimp_SetMode( &vid.width, &vid.height, gl_mode->value, false ) ) == rserr_ok )
				return true;
		}
		else if ( err == rserr_invalid_mode )
		{
			ri.Cvar_SetValue( "gl_mode", gx_state.prev_mode );
			gl_mode->modified = false;
			ri.Con_Printf( PRINT_ALL, "ref_gx::R_SetMode() - invalid mode\n" );
		}

		// try setting it back to something safe
		if ( ( err = GXimp_SetMode( &vid.width, &vid.height, gx_state.prev_mode, false ) ) != rserr_ok )
		{
			ri.Con_Printf( PRINT_ALL, "ref_gx::R_SetMode() - could not revert to safe mode\n" );
			return false;
		}
	}
	return true;
}

/*
===============
R_Init
===============
*/
qboolean R_Init( void *hinstance, void *hWnd )
{	
	int		err;
	int		j;
	extern float r_turbsin[256];

	memset(gxtexobjs, 0, sizeof(gxtexobjs));

	for ( j = 0; j < 256; j++ )
	{
		r_turbsin[j] *= 0.5;
	}

	ri.Con_Printf (PRINT_ALL, "ref_gx version: "REF_VERSION"\n");

	Draw_GetPalette ();

	R_Register();

	// initialize our QGX dynamic bindings
	if ( !QGX_Init( gl_driver->string ) )
	{
		QGX_Shutdown();
        ri.Con_Printf (PRINT_ALL, "ref_gx::R_Init() - could not load \"%s\"\n", gl_driver->string );
		return -1;
	}

	// initialize GX specific parts
	if ( !GXimp_Init( hinstance, hWnd ) )
	{
		QGX_Shutdown();
		return -1;
	}

	// set our "safe" modes
	gx_state.prev_mode = 3;

	// create the window and set up the context
	if ( !R_SetMode () )
	{
		QGX_Shutdown();
        ri.Con_Printf (PRINT_ALL, "ref_gx::R_Init() - could not R_SetMode()\n" );
		return -1;
	}

	ri.Vid_MenuInit();

	if ( toupper( gl_monolightmap->string[1] ) != 'F' )
	{
		ri.Cvar_Set( "gl_monolightmap", "0" );
	}

	ri.Cvar_Set( "scr_drawall", "0" );

	/*
	** grab extensions
	*/

#ifdef _WIN32
	if ( strstr( gx_config.extensions_string, "WGL_EXT_swap_control" ) )
	{
		qwglSwapIntervalEXT = ( BOOL (WINAPI *)(int)) qwglGetProcAddress( "wglSwapIntervalEXT" );
		ri.Con_Printf( PRINT_ALL, "...enabling WGL_EXT_swap_control\n" );
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "...WGL_EXT_swap_control not found\n" );
	}
#endif

	/*if ( gl_ext_pointparameters->value )
	{

		qgxSetPointSize = GX_SetPointSize;
		ri.Con_Printf( PRINT_ALL, "...using point size parameters\n" );
	}
	else
	{*/
		ri.Con_Printf( PRINT_ALL, "...ignoring point size parameters\n" );
	/*}*/

	/*if ( gl_ext_palettedtexture->value )
	{
		ri.Con_Printf( PRINT_ALL, "...using color index mode\n" );
		qgxInitTlutObj = GX_InitTlutObj;
		qgxLoadTlut = GX_LoadTlut;
	}
	else
	{*/
		ri.Con_Printf( PRINT_ALL, "...ignoring color index mode\n" );
	/*}*/

	if ( gl_ext_multitexture->value )
	{
		ri.Con_Printf( PRINT_ALL, "...using multitexture\n" );
		GX_TEXTURE0 = GX_TEXMAP0;
		GX_TEXTURE1 = GX_TEXMAP1;
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "...ignoring multitexture\n" );
		GX_TEXTURE0 = GX_TEXMAP0;
		GX_TEXTURE1 = GX_TEXMAP0;
	}

	GX_SetDefaultState();

	/*
	** draw our stereo patterns
	*/
#if 0 // commented out until H3D pays us the money they owe us
	GL_DrawStereoPattern();
#endif

	GX_InitImages ();
	Mod_Init ();
	R_InitParticleTexture ();
	Draw_InitLocal ();

	return 1;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{	
	ri.Cmd_RemoveCommand ("modellist");
	ri.Cmd_RemoveCommand ("screenshot");
	ri.Cmd_RemoveCommand ("imagelist");
	ri.Cmd_RemoveCommand ("gl_strings");

	Mod_FreeAll ();

	GX_ShutdownImages ();

	/*
	** shut down GX specific stuff
	*/
	GXimp_Shutdown();

	/*
	** shutdown our QGX subsystem
	*/
	QGX_Shutdown();
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame( float camera_separation )
{

	gx_state.camera_separation = camera_separation;

	/*
	** change modes if necessary
	*/
	if ( gl_mode->modified || vid_fullscreen->modified )
	{	// FIXME: only restart if CDS is required
		cvar_t	*ref;

		ref = ri.Cvar_Get ("vid_ref", "gl", 0);
		ref->modified = true;
	}

	if ( gl_log->modified )
	{
		GXimp_EnableLogging( gl_log->value );
		gl_log->modified = false;
	}

	if ( gl_log->value )
	{
		GXimp_LogNewFrame();
	}

	if ( vid_gamma->modified )
	{
		vid_gamma->modified = false;
	}

	GXimp_BeginFrame( camera_separation );

	/*
	** go into 2D mode
	*/
	gxu_viewport_x = (sys_rmode->viWidth - vid.width) / 2;
	gxu_viewport_y = (sys_rmode->viHeight - vid.height) / 2;
	gxu_viewport_width = vid.width;
	gxu_viewport_height = vid.height;
	qgxSetScissor(gxu_viewport_x, gxu_viewport_y, gxu_viewport_width, gxu_viewport_height);
	qgxSetViewport (gxu_viewport_x, gxu_viewport_y, gxu_viewport_width, gxu_viewport_height, gxu_depth_min, gxu_depth_max);
	qguOrtho(gxu_projection_matrices[gxu_cur_projection_matrix], 0, vid.height, 0, vid.width, GXU_ORTHO_ZNEAR, GXU_ORTHO_ZFAR);
	gxu_cur_projection_type = GX_ORTHOGRAPHIC;
	qgxLoadProjectionMtx(gxu_projection_matrices[gxu_cur_projection_matrix], gxu_cur_projection_type);
	qguMtxIdentity(gxu_modelview_matrices[gxu_cur_modelview_matrix]);
	qgxLoadPosMtxImm(gxu_modelview_matrices[gxu_cur_modelview_matrix], GX_PNMTX0);
	gxu_z_test_enabled = GX_FALSE;
	qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
	gxu_cull_enabled = false;
	qgxSetCullMode(GX_CULL_NONE);
	qgxSetBlendMode(GX_BM_NONE, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
	qgxSetAlphaCompare(GX_GEQUAL, gxu_alpha_test_lower, GX_AOP_AND, GX_LEQUAL, gxu_alpha_test_higher);
	gxu_cur_r = 255;
	gxu_cur_g = 255;
	gxu_cur_b = 255;
	gxu_cur_a = 255;

	/*
	** draw buffer stuff
	*/
	if ( gl_drawbuffer->modified )
	{
		gl_drawbuffer->modified = false;

		// Implement this ASAP:
		//if ( gx_state.camera_separation == 0 || !gx_state.stereo_enabled )
		//{
		//	if ( Q_stricmp( gl_drawbuffer->string, "GL_FRONT" ) == 0 )
		//		qglDrawBuffer( GL_FRONT );
		//	else
		//		qglDrawBuffer( GL_BACK );
		//}
	}

	/*
	** texturemode stuff
	*/
	if ( gl_texturemode->modified )
	{
		GX_TextureMode( gl_texturemode->string );
		gl_texturemode->modified = false;
	}

	if ( gl_texturealphamode->modified )
	{
		GX_TextureAlphaMode( gl_texturealphamode->string );
		gl_texturealphamode->modified = false;
	}

	if ( gl_texturesolidmode->modified )
	{
		GX_TextureSolidMode( gl_texturesolidmode->string );
		gl_texturesolidmode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GX_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear ();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette ( const unsigned char *palette)
{
	int		i;

	byte *rp = ( byte * ) r_rawpalette;

	if ( palette )
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = palette[i*3+0];
			rp[i*4+1] = palette[i*3+1];
			rp[i*4+2] = palette[i*3+2];
			rp[i*4+3] = 0xff;
		}
	}
	else
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = ( d_8to24table[i] >> 24 ) & 0xff;
			rp[i*4+1] = ( d_8to24table[i] >> 16 ) & 0xff;
			rp[i*4+2] = ( d_8to24table[i] >> 8 ) & 0xff;
			rp[i*4+3] = 0xff;
		}
	}
	GX_SetTexturePalette( r_rawpalette );

	gxu_cur_projection_matrix++;
	qguOrtho(gxu_projection_matrices[gxu_cur_projection_matrix], 0, vid.height, 0, vid.width, GXU_ORTHO_ZNEAR, GXU_ORTHO_ZFAR);
	qgxLoadProjectionMtx(gxu_projection_matrices[gxu_cur_projection_matrix], GX_ORTHOGRAPHIC);
	qgxDisableTexture();
	qgxBegin (GX_QUADS, GX_VTXFMT0, 4);
	qgxPosition3f32 (0, r_newrefdef.y, GXU_ORTHO_ZCOORD_BOTTOM);
	qgxColor4u8 (0, 0, 0, 0);
	qgxPosition3f32 (vid.width, 0, GXU_ORTHO_ZCOORD_BOTTOM);
	qgxColor4u8 (0, 0, 0, 0);
	qgxPosition3f32 (vid.width, vid.height, GXU_ORTHO_ZCOORD_BOTTOM);
	qgxColor4u8 (0, 0, 0, 0);
	qgxPosition3f32 (0, vid.height, GXU_ORTHO_ZCOORD_BOTTOM);
	qgxColor4u8 (0, 0, 0, 0);
	qgxEnd ();
	qgxEnableTexture();
	gxu_cur_projection_matrix--;
	qgxLoadProjectionMtx(gxu_projection_matrices[gxu_cur_projection_matrix], gxu_cur_projection_type);
}

/*
** R_DrawBeam
*/
void R_DrawBeam( entity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	qgxDisableTexture();
	qgxSetBlendMode(GX_BM_BLEND, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
	gxu_z_write_enabled = GX_FALSE;
	qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);

	gxu_cur_r = ( d_8to24table[e->skinnum & 0xFF] >> 24 ) & 0xFF;
	gxu_cur_g = ( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF;
	gxu_cur_b = ( d_8to24table[e->skinnum & 0xFF] >> 8  ) & 0xFF;
	gxu_cur_a = e->alpha * 255;

	qgxBegin (GX_TRIANGLESTRIP, GX_VTXFMT0, NUM_BEAM_SEGS * 4);
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		qgxPosition3f32( start_points[i][0], start_points[i][1], start_points[i][2] );
		qgxColor4u8( gxu_cur_r, gxu_cur_g, gxu_cur_b, gxu_cur_a );
		qgxPosition3f32( end_points[i][0], end_points[i][1], end_points[i][2] );
		qgxColor4u8( gxu_cur_r, gxu_cur_g, gxu_cur_b, gxu_cur_a );
		qgxPosition3f32( start_points[(i+1)%NUM_BEAM_SEGS][0], start_points[(i+1)%NUM_BEAM_SEGS][1], start_points[(i+1)%NUM_BEAM_SEGS][2] );
		qgxColor4u8( gxu_cur_r, gxu_cur_g, gxu_cur_b, gxu_cur_a );
		qgxPosition3f32( end_points[(i+1)%NUM_BEAM_SEGS][0], end_points[(i+1)%NUM_BEAM_SEGS][1], end_points[(i+1)%NUM_BEAM_SEGS][2] );
		qgxColor4u8( gxu_cur_r, gxu_cur_g, gxu_cur_b, gxu_cur_a );
	}
	qgxEnd();

	qgxEnableTexture();
	qgxSetBlendMode(GX_BM_NONE, gxu_blend_src_value, gxu_blend_dst_value, GX_LO_NOOP);
	gxu_z_write_enabled = GX_TRUE;
	qgxSetZMode(gxu_z_test_enabled, gxu_cur_z_func, gxu_z_write_enabled);
}

//===================================================================


void	R_BeginRegistration (char *map);
struct model_s	*R_RegisterModel (char *name);
struct image_s	*R_RegisterSkin (char *name);
void R_SetSky (char *name, float rotate, vec3_t axis);
void	R_EndRegistration (void);

void	R_RenderFrame (refdef_t *fd);

struct image_s	*Draw_FindPic (char *name);

void	Draw_Pic (int x, int y, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI (refimport_t rimp )
{
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.BeginRegistration = R_BeginRegistration;
	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = Draw_FindPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen= Draw_FadeScreen;

	re.DrawStretchRaw = Draw_StretchRaw;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.CinematicSetPalette = R_SetPalette;
	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GXimp_EndFrame;

	re.AppActivate = GXimp_AppActivate;

	Swap_Init ();

	return re;
}


#ifndef REF_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	ri.Sys_Error (ERR_FATAL, "%s", text);
}

void Com_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	ri.Con_Printf (PRINT_ALL, "%s", text);
}

#endif
