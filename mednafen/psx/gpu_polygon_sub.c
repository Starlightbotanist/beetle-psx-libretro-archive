/*
 * gpu_polygon_sub.c - polygon-helper free functions used by the
 * software rasteriser's polygon decoder.
 *
 * Switched to a minimal include set: gpu.h gives us PS_GPU and
 * tri_vertex (both now C-includable as of 7d257de), rhi_intf.h
 * gives us RHI_VULKAN and rhi_intf_is_type, and
 * beetle_psx_globals.h gives us psx_gpu_upscale_shift_hw.
 */

#include "gpu.h"
#include "../../rhi/rhi_intf.h"
#include "../../beetle_psx_globals.h"

#include <retro_miscellaneous.h>
#include <inttypes.h>
#include <string.h>

extern retro_log_printf_t log_cb;

/* Buffer the whole burst before logging, so file I/O cannot intervene
 * between the polygon decoder's reads and writes. Diagnostic state is
 * deliberately separate from PS_GPU and is not part of a savestate. */
#define UV_DIAG_FRAMES 12
#define UV_DIAG_PER_FRAME 2048

struct uv_diag_record
{
	tri_vertex vertices[3];
	int64_t area, tex_area, dudx, dudy, dvdx, dvdy;
	unsigned frame, sequence, count, command, continuation;
	unsigned page_x, page_y, mode, clut, window_x, window_y;
	unsigned inherited_u, inherited_v, inherited_2d;
	unsigned off_u, off_v, may_be_2d, is_3d;
	unsigned min_u, min_v, max_u, max_v, has_limits;
};

static struct uv_diag_record uv_diag_records[UV_DIAG_FRAMES * UV_DIAG_PER_FRAME];
static unsigned uv_diag_frame, uv_diag_burst, uv_diag_remaining;
static unsigned uv_diag_used, uv_diag_seen, uv_diag_dropped;
static bool uv_diag_marker_down;
static struct uv_diag_record *uv_diag_pending;

void GPU_UVDiagReset(void)
{
	if (uv_diag_remaining && log_cb)
		log_cb(RETRO_LOG_INFO, "[UV-DIAG] aborted burst=%u records=%u\n",
				uv_diag_burst, uv_diag_used);
	uv_diag_frame = uv_diag_burst = uv_diag_remaining = 0;
	uv_diag_used = uv_diag_seen = uv_diag_dropped = 0;
	uv_diag_marker_down = false;
	uv_diag_pending = NULL;
}

bool GPU_UVDiagBeginFrame(bool marker)
{
	bool started = marker && !uv_diag_marker_down && !uv_diag_remaining;
	uv_diag_marker_down = marker;
	uv_diag_frame++;
	uv_diag_seen = 0;
	uv_diag_pending = NULL;
	if (started)
	{
		uv_diag_burst++;
		uv_diag_remaining = UV_DIAG_FRAMES;
		uv_diag_used = uv_diag_dropped = 0;
		if (log_cb)
			log_cb(RETRO_LOG_INFO,
					"[UV-DIAG] begin burst=%u frame=%u frames=%u cap_per_frame=%u\n",
					uv_diag_burst, uv_diag_frame, UV_DIAG_FRAMES, UV_DIAG_PER_FRAME);
	}
	return started;
}

void GPU_UVDiagEndFrame(void)
{
	unsigned i;
	uv_diag_pending = NULL;
	if (!uv_diag_remaining || --uv_diag_remaining)
		return;
	if (!log_cb)
		return;
	for (i = 0; i < uv_diag_used; i++)
	{
		const struct uv_diag_record *r = &uv_diag_records[i];
		log_cb(RETRO_LOG_INFO,
				"[UV-DIAG] draw burst=%u f=%u seq=%u n=%u cmd=%02x continuation=%u "
				"page=%u,%u mode=%u clut=%u window=%u,%u "
				"inherited=%u,%u,%u off=%u,%u sprite=%u depth_varies=%u "
				"area=%" PRId64 " texarea=%" PRId64 " deriv=%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 " "
				"limits_valid=%u limits=%u,%u,%u,%u "
				"a=%d,%d,%d,%d/%.6g,%.6g,%.6g "
				"b=%d,%d,%d,%d/%.6g,%.6g,%.6g "
				"c=%d,%d,%d,%d/%.6g,%.6g,%.6g\n",
				uv_diag_burst, r->frame, r->sequence, r->count, r->command, r->continuation,
				r->page_x, r->page_y, r->mode, r->clut, r->window_x, r->window_y,
				r->inherited_u, r->inherited_v, r->inherited_2d,
				r->off_u, r->off_v, r->may_be_2d, r->is_3d,
				r->area, r->tex_area, r->dudx, r->dudy, r->dvdx, r->dvdy,
				r->has_limits, r->min_u, r->min_v, r->max_u, r->max_v,
				r->vertices[0].x, r->vertices[0].y, r->vertices[0].u, r->vertices[0].v,
				r->vertices[0].precise[0], r->vertices[0].precise[1], r->vertices[0].precise[2],
				r->vertices[1].x, r->vertices[1].y, r->vertices[1].u, r->vertices[1].v,
				r->vertices[1].precise[0], r->vertices[1].precise[1], r->vertices[1].precise[2],
				r->vertices[2].x, r->vertices[2].y, r->vertices[2].u, r->vertices[2].v,
				r->vertices[2].precise[0], r->vertices[2].precise[1], r->vertices[2].precise[2]);
	}
	log_cb(RETRO_LOG_INFO, "[UV-DIAG] end burst=%u records=%u dropped=%u\n",
			uv_diag_burst, uv_diag_used, uv_diag_dropped);
}

/* Determine whether to offset UVs to account for difference in interpolation between PS1 and modern GPUs */
void Calc_UVOffsets_Adjust_Verts(PS_GPU *gpu, tri_vertex *vertices, unsigned count)
{
	/* iCB: Just borrowing this from \parallel-psx\renderer\renderer.cpp */
	uint16_t off_u = 0;
	uint16_t off_v = 0;
	bool may_be_2d = false;
	if (gpu->InCmd == INCMD_QUAD)
	{
		off_u = gpu->off_u;
		off_v = gpu->off_v;
		may_be_2d = gpu->may_be_2d;
	}

	/* For X/Y flipped 2D sprites, PSX games rely on a very specific rasterization behavior.
	 * If U or V is decreasing in X or Y, and we use the provided U/V as is, we will sample the wrong texel as interpolation
	 * covers an entire pixel, while PSX samples its interpolation essentially in the top-left corner and splats that interpolant across the entire pixel.
	 * While we could emulate this reasonably well in native resolution by shifting our vertex coords by 0.5,
	 * this breaks in upscaling scenarios, because we have several samples per native sample and we need NN rules to hit the same UV every time.
	 * One approach here is to use interpolate at offset or similar tricks to generalize the PSX interpolation patterns,
	 * but the problem is that vertices sharing an edge will no longer see the same UV (due to different plane derivatives),
	 * we end up sampling outside the intended boundary and artifacts are inevitable, so the only case where we can apply this fixup is for "sprites"
	 * or similar which should not share edges, which leads to this unfortunate code below.
	 */
	{
		/* It might be faster to do more direct checking here, but the code below handles primitives in any order
		 * and orientation, and is far more SIMD-friendly if needed.
		 *
		 * All inputs (x, y, u, v) are int32_t, so every derivative below is
		 * integer-valued.  It used to be computed in float: at upscale_shift
		 * == 0 the coords are small and the float math is exact, but with the
		 * software renderer upscaling (upscale_shift > 0) the coords are
		 * shifted up and the mul-add chains overflow 2^24, at which point the
		 * < 0 / == 0 tests round - and round differently under FMA contraction
		 * or x87-vs-SSE codegen, so off_u/off_v/may_be_2d (and the Wild Arms 2
		 * vertex fixup) could diverge between netplay peers.  Compute the
		 * derivatives exactly in int64 instead; the results feed only sign and
		 * zero tests, so the old `* inv_area` scaling was a no-op for the
		 * decisions (it cannot change a sign or a zero) and is dropped. */
		int64_t abx = (int64_t)vertices[1].x - vertices[0].x;
		int64_t aby = (int64_t)vertices[1].y - vertices[0].y;
		int64_t bcx = (int64_t)vertices[2].x - vertices[1].x;
		int64_t bcy = (int64_t)vertices[2].y - vertices[1].y;
		int64_t cax = (int64_t)vertices[0].x - vertices[2].x;
		int64_t cay = (int64_t)vertices[0].y - vertices[2].y;

		/* Compute static derivatives, just assume W is uniform across the primitive
		 * and that the plane equation remains the same across the quad. */
		int64_t dudx = -aby * vertices[2].u - bcy * vertices[0].u - cay * vertices[1].u;
		int64_t dvdx = -aby * vertices[2].v - bcy * vertices[0].v - cay * vertices[1].v;
		int64_t dudy = +abx * vertices[2].u + bcx * vertices[0].u + cax * vertices[1].u;
		int64_t dvdy = +abx * vertices[2].v + bcx * vertices[0].v + cax * vertices[1].v;
		int64_t area = bcx * cay - bcy * cax;

		/* iCB: Detect and reject any triangles with 0 size texture area */
		int64_t texArea = ((int64_t)vertices[1].u - vertices[0].u) * (vertices[2].v - vertices[0].v) - ((int64_t)vertices[2].u - vertices[0].u) * (vertices[1].v - vertices[0].v);

		/* Leverage PGXP to further avoid 3D polygons that just happen to align this way after projection */
		bool is3D = ((vertices[0].precise[2] != vertices[1].precise[2]) || (vertices[1].precise[2] != vertices[2].precise[2]));

		uv_diag_pending = NULL;
		if (uv_diag_remaining)
		{
			if (uv_diag_seen++ < UV_DIAG_PER_FRAME)
			{
				struct uv_diag_record *r = &uv_diag_records[uv_diag_used++];
				memset(r, 0, sizeof(*r));
				memcpy(r->vertices, vertices, sizeof(r->vertices));
				r->frame = uv_diag_frame;
				r->sequence = uv_diag_seen;
				r->count = count;
				r->command = gpu->InCmd_CC;
				r->continuation = gpu->InCmd == INCMD_QUAD;
				r->page_x = gpu->TexPageX;
				r->page_y = gpu->TexPageY;
				r->mode = gpu->TexMode;
				r->clut = gpu->CLUT_Cache_VB;
				r->window_x = gpu->SUCV.TWX_AND;
				r->window_y = gpu->SUCV.TWY_AND;
				r->inherited_u = off_u;
				r->inherited_v = off_v;
				r->inherited_2d = may_be_2d;
				r->area = area;
				r->tex_area = texArea;
				r->dudx = dudx;
				r->dudy = dudy;
				r->dvdx = dvdx;
				r->dvdy = dvdy;
				r->is_3d = is3D;
				uv_diag_pending = r;
			}
			else
				uv_diag_dropped++;
		}

		/* Shouldn't matter as degenerate primitives will be culled anyways. */
		if ((area != 0) && (texArea != 0) && !is3D)
		{
			/* Sign of (deriv / inv_area) equals (sign of deriv) XOR (sign
			 * of area); zero-ness is unchanged since inv_area is non-zero. */
			bool neg_area  = area < 0;
			bool neg_dudx  = (dudx < 0) != neg_area;
			bool neg_dudy  = (dudy < 0) != neg_area;
			bool neg_dvdx  = (dvdx < 0) != neg_area;
			bool neg_dvdy  = (dvdy < 0) != neg_area;
			bool zero_dudx = dudx == 0;
			bool zero_dudy = dudy == 0;
			bool zero_dvdx = dvdx == 0;
			bool zero_dvdy = dvdy == 0;

			/* Dumb heuristic to check if a polygon may be 2D */
			may_be_2d = may_be_2d || zero_dudy || zero_dudx || zero_dvdy || zero_dvdx;

			/* If we have negative dU or dV in any direction, increment the U or V to work properly with nearest-neighbor in this impl.
			 * If we don't have 1:1 pixel correspondence, this creates a slight "shift" in the sprite, but we guarantee that we don't sample garbage at least.
			 * Overall, this is kinda hacky because there can be legitimate, rare cases where 3D meshes hit this scenario, and a single texel offset can pop in, but
			 * this is way better than having borked 2D overall.
			 * TODO: Try to figure out if this can be generalized.
			 *
			 * TODO: If perf becomes an issue, we can probably SIMD the 8 comparisons above,
			 * create an 8-bit code, and use a LUT to get the offsets.
			 * Case 1: U is decreasing in X, but no change in Y.
			 * Case 2: U is decreasing in Y, but no change in X.
			 * Case 3: V is decreasing in X, but no change in Y.
			 * Case 4: V is decreasing in Y, but no change in X. */
			if (rhi_intf_is_type() != RHI_VULKAN || psx_gpu_upscale_shift_hw)
			{
				if (neg_dudx && zero_dudy)
					off_u = 1;
				else if (neg_dudy && zero_dudx)
					off_u = 1;
				if (neg_dvdx && zero_dvdy)
					off_v = 1;
				else if (neg_dvdy && zero_dvdx)
					off_v = 1;
			}

			/* HACK fix Wild Arms 2 overworld forest sprite
			 * TODO generalize this perhaps? */
			{
				const int64_t one = (int64_t)1 << gpu->upscale_shift;
				if (zero_dvdx &&
					(aby == one || bcy == one || cay == one) &&
					(aby == 0 || bcy == 0 || cay == 0) &&
					(aby == -one || bcy == -one || cay == -one)
				)
				{
					if (neg_dvdy)
					{
						if (aby == -one)
							vertices[0].v = vertices[1].v - 1;
						else if (bcy == -one)
							vertices[1].v = vertices[2].v - 1;
						else if (cay == -one)
							vertices[2].v = vertices[0].v - 1;

						if (aby == one)
							vertices[1].v = vertices[0].v - 1;
						else if (bcy == one)
							vertices[2].v = vertices[1].v - 1;
						else if (cay == one)
							vertices[0].v = vertices[2].v - 1;
					}
				}
			}
		}
	}

	gpu->off_u = off_u;
	gpu->off_v = off_v;
	gpu->may_be_2d = may_be_2d;
	if (uv_diag_pending)
	{
		uv_diag_pending->off_u = off_u;
		uv_diag_pending->off_v = off_v;
		uv_diag_pending->may_be_2d = may_be_2d;
	}
}

/* Reset min/max UVs for primitive */
void Reset_UVLimits(PS_GPU *gpu)
{
	gpu->min_u = UINT16_MAX;
	gpu->min_v = UINT16_MAX;
	gpu->max_u = 0;
	gpu->max_v = 0;
}

/* Determine min and max UVs sampled for a given primitive */
void Extend_UVLimits(PS_GPU *gpu, tri_vertex *vertices, unsigned count)
{
	uint8_t twx = gpu->SUCV.TWX_AND;
	uint8_t twy = gpu->SUCV.TWY_AND;

	uint16_t min_u = gpu->min_u;
	uint16_t min_v = gpu->min_v;
	uint16_t max_u = gpu->max_u;
	uint16_t max_v = gpu->max_v;

	if ((twx == (uint8_t)0xffu) && (twy == (uint8_t)0xffu))
	{
		unsigned int i;
		/* If we're not using texture window, we're likely accessing a small subset of the texture. */
		for (i = 0; i < count; i++)
		{
			uint16_t u = (uint16_t)vertices[i].u;
			uint16_t v = (uint16_t)vertices[i].v;
			if (u < min_u) min_u = u;
			if (v < min_v) min_v = v;
			if (u > max_u) max_u = u;
			if (v > max_v) max_v = v;
		}
	}
	else
	{
		/* texture window so don't clamp texture */
		min_u = 0;
		min_v = 0;
		max_u = UINT16_MAX;
		max_v = UINT16_MAX;
	}

	gpu->min_u = min_u;
	gpu->min_v = min_v;
	gpu->max_u = max_u;
	gpu->max_v = max_v;
}

/* Apply offsets to UV limits before returning */
void Finalise_UVLimits(PS_GPU *gpu)
{
	uint8_t twx = gpu->SUCV.TWX_AND;
	uint8_t twy = gpu->SUCV.TWY_AND;

	uint16_t min_u = gpu->min_u;
	uint16_t min_v = gpu->min_v;
	uint16_t max_u = gpu->max_u;
	uint16_t max_v = gpu->max_v;

	uint16_t off_u = gpu->off_u;
	uint16_t off_v = gpu->off_v;

	if ((twx == (uint8_t)0xffu) && (twy == (uint8_t)0xffu))
	{
		/* offset output UV Limits */
		min_u += off_u;
		min_v += off_v;
		max_u += off_u;
		max_v += off_v;

		/* In nearest neighbor, we'll get *very* close to this UV, but not close enough to actually sample it.
		 * If du/dx or dv/dx are negative, we probably need to invert this though ... */
		if ((rhi_intf_is_type() != RHI_VULKAN || psx_gpu_upscale_shift_hw) && gpu->may_be_2d)
		{
			if (max_u > min_u)
				max_u--;
			if (max_v > min_v)
				max_v--;
		}

		/* If there's no wrapping, we can prewrap and avoid fallback. */
		if ((max_u & 0xff00) == (min_u & 0xff00))
			max_u &= 0xff;
		if ((max_v & 0xff00) == (min_v & 0xff00))
			max_v &= 0xff;
	}
	else
	{
		/* texture window so don't clamp texture */
		min_u = 0;
		min_v = 0;
		max_u = UINT16_MAX;
		max_v = UINT16_MAX;
	}

	gpu->min_u = min_u;
	gpu->min_v = min_v;
	gpu->max_u = max_u;
	gpu->max_v = max_v;
	if (uv_diag_pending)
	{
		uv_diag_pending->min_u = min_u;
		uv_diag_pending->min_v = min_v;
		uv_diag_pending->max_u = max_u;
		uv_diag_pending->max_v = max_v;
		uv_diag_pending->has_limits = true;
		uv_diag_pending = NULL;
	}
}


/* 0 = disabled
 * 1 = enabled (default mode)
 * 2 = enabled (aggressive mode) */

/* Hack to deal with PS1 games rendering axis aligned lines using 1 pixel wide triangles with UVs that describe a line
 * Suitable for games like Soul Blade, Doom and Hexen */
bool Hack_FindLine(PS_GPU *gpu, tri_vertex* vertices, tri_vertex* outVertices)
{
	int32_t pxWidth = 1 << gpu->upscale_shift;	/* width of a single pixel */
	uint8_t cornerIdx, shortIdx, longIdx;

	/* reject 3D elements */
	if ((vertices[0].precise[2] != vertices[1].precise[2]) ||
		(vertices[1].precise[2] != vertices[2].precise[2]))
		return false;

	/* find short side of triangle / end of line with 2 vertices (guess which vertex is the right angle) */
	if ((vertices[0].u == vertices[1].u) && (vertices[0].v == vertices[1].v))
		cornerIdx = 0;
	else if ((vertices[1].u == vertices[2].u) && (vertices[1].v == vertices[2].v))
		cornerIdx = 1;
	else if ((vertices[2].u == vertices[0].u) && (vertices[2].v == vertices[0].v))
		cornerIdx = 2;
	else
		return false;

	/* assign other indices to remaining vertices */
	shortIdx = (cornerIdx + 1) % 3;
	longIdx = (shortIdx + 1) % 3;

	/* determine line orientation and check width */
	if ((vertices[cornerIdx].x == vertices[shortIdx].x) && (abs(vertices[cornerIdx].y - vertices[shortIdx].y) == pxWidth))
	{
		/* line is horizontal
		 * determine which is truly the corner by checking against the long side, while making sure it is axis aligned */
		if (vertices[shortIdx].y == vertices[longIdx].y)
		{
			uint8_t tempIdx = shortIdx;
			shortIdx = cornerIdx;
			cornerIdx = tempIdx;
		}
		else if (vertices[cornerIdx].y != vertices[longIdx].y)
			return false;

		/* flip corner index to other side of quad */
		outVertices[cornerIdx] = vertices[longIdx];
		outVertices[cornerIdx].y = vertices[shortIdx].y;
		outVertices[cornerIdx].precise[1] = vertices[shortIdx].precise[1];
	}
	else if ((vertices[cornerIdx].y == vertices[shortIdx].y) && (abs(vertices[cornerIdx].x - vertices[shortIdx].x) == pxWidth))
	{
		/* line is vertical
		 * determine which is truly the corner by checking against the long side, while making sure it is axis aligned */
		if (vertices[shortIdx].x == vertices[longIdx].x)
		{
			uint8_t tempIdx = shortIdx;
			shortIdx = cornerIdx;
			cornerIdx = tempIdx;
		}
		else if (vertices[cornerIdx].x != vertices[longIdx].x)
			return false;

		/* flip corner index to other side of quad */
		outVertices[cornerIdx] = vertices[longIdx];
		outVertices[cornerIdx].x = vertices[shortIdx].x;
		outVertices[cornerIdx].precise[0] = vertices[shortIdx].precise[0];
	}
	else
		return false;

	outVertices[shortIdx] = vertices[shortIdx];
	outVertices[longIdx] = vertices[longIdx];

	return true;
}

/* Hack to deal with PS1 games rendering axis aligned lines using 1 pixel wide triangles and force UVs to describe a line
 * Required for games like Dark Forces and Duke Nukem */
bool Hack_ForceLine(PS_GPU *gpu, tri_vertex* vertices, tri_vertex* outVertices)
{
	int32_t pxWidth = 1 << gpu->upscale_shift;	/* width of a single pixel */
	uint8_t cornerIdx, shortIdx, longIdx;
	uint8_t A, B, C;

	/* reject 3D elements */
	if ((vertices[0].precise[2] != vertices[1].precise[2]) ||
		(vertices[1].precise[2] != vertices[2].precise[2]))
		return false;

	/* find vertical AB */
	if (vertices[0].x == vertices[1].x)
		A = 0;
	else if (vertices[1].x == vertices[2].x)
		A = 1;
	else if (vertices[2].x == vertices[0].x)
		A = 2;
	else
		return false;

	/* assign other indices to remaining vertices */
	B = (A + 1) % 3;
	C = (B + 1) % 3;

	/* find horizontal AC or BC */
	if (vertices[A].y == vertices[C].y)
		cornerIdx = A;
	else if (vertices[B].y == vertices[C].y)
		cornerIdx = B;
	else
		return false;

	/* determine lengths of sides */
	if (abs(vertices[A].y - vertices[B].y) == pxWidth)
	{
		/* is Horizontal */
		shortIdx = (cornerIdx == A) ? B : A;
		longIdx = C;

		/* flip corner index to other side of quad */
		outVertices[cornerIdx] = vertices[longIdx];
		outVertices[cornerIdx].y = vertices[shortIdx].y;
		outVertices[cornerIdx].precise[1] = vertices[shortIdx].precise[1];
	}
	else if (abs(vertices[A].x - vertices[C].x) == pxWidth)
	{
		/* is Vertical */
		shortIdx = C;
		longIdx = (cornerIdx == A) ? B : A;

		/* flip corner index to other side of quad */
		outVertices[cornerIdx] = vertices[longIdx];
		outVertices[cornerIdx].x = vertices[shortIdx].x;
		outVertices[cornerIdx].precise[0] = vertices[shortIdx].precise[0];
	}
	else
		return false;

	/* force UVs into a line along the upper or left most edge of the triangle
	 * Otherwise the wrong UVs will be sampled on second triangle and by hardware renderers */
	vertices[shortIdx].u = vertices[cornerIdx].u;
	vertices[shortIdx].v = vertices[cornerIdx].v;

	/* copy other two vertices */
	outVertices[shortIdx] = vertices[shortIdx];
	outVertices[longIdx] = vertices[longIdx];

	return true;
}
