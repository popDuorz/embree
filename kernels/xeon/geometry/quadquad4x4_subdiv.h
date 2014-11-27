// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "quadquad4x4.h"

namespace embree
{
  struct QuadQuad4x4AdaptiveSubdivision
  {
    PrimRef* prims_o;
    FastAllocator& alloc;
    Scene* scene;
    const unsigned int geomID;
    const unsigned int primID;
    size_t count;

    QuadQuad4x4AdaptiveSubdivision (PrimRef* prims_o,
                                    FastAllocator& alloc,
                                    Scene* scene, // FIXME: pass displacement shader instead
                                    const SubdivMesh::HalfEdge* h, 
                                    const Vec3fa* vertices, 
                                    const unsigned int geomID, 
                                    const unsigned int primID)
    : prims_o(prims_o), alloc(alloc), scene(scene), geomID(geomID), primID(primID), count(0)
      {
#if 1
        const CatmullClarkPatch patch(h,vertices);
        const bool subdiv0 = !h->hasOpposite() || !h->opposite()->dicable(); h = h->next(); // FIXME: should be false if no neighbour?
        const bool subdiv1 = !h->hasOpposite() || !h->opposite()->dicable(); h = h->next();
        const bool subdiv2 = !h->hasOpposite() || !h->opposite()->dicable(); h = h->next();
        const bool subdiv3 = !h->hasOpposite() || !h->opposite()->dicable(); h = h->next();
        subdivide(patch,5,Vec2f(0.0f,0.0f),Vec2f(0.0f,1.0f),Vec2f(1.0f,1.0f),Vec2f(1.0f,0.0f),subdiv0,subdiv1,subdiv2,subdiv3);
#else
	bool subdiv[GeneralCatmullClarkPatch::SIZE];
        const GeneralCatmullClarkPatch patch(h,vertices);
	for (size_t i=0; i<patch.size(); i++) {
	  subdiv[i] = !h->hasOpposite() || !h->opposite()->dicable(); h = h->next();
	}
        subdivide(patch,5,subdiv);
#endif
      }

    __forceinline size_t size() const {
      return count;
    }

    void subdivide(const GeneralCatmullClarkPatch& patch, int depth, bool subdiv[GeneralCatmullClarkPatch::SIZE])
    {
#if 1
      if (patch.size() == 4) {
	CatmullClarkPatch qpatch; patch.init(qpatch);
	subdivide(qpatch,depth,Vec2f(0.0f,0.0f),Vec2f(0.0f,1.0f),Vec2f(1.0f,1.0f),Vec2f(1.0f,0.0f),subdiv[0],subdiv[1],subdiv[2],subdiv[3]);
	return;
      }
#endif

      size_t N;
      CatmullClarkPatch patches[GeneralCatmullClarkPatch::SIZE]; 
      patch.subdivide(patches,N);

      const bool noleaf = depth > 1;
      bool csubdiv[GeneralCatmullClarkPatch::SIZE];
      for (size_t i=0; i<N; i++)
        csubdiv[i] = noleaf && !patches[i].dicable();

      /* parametrization for triangles */
      if (N == 3) {
	const Vec2f uv_0(0.0f,0.0f);
	const Vec2f uv01(0.5f,0.0f);
	const Vec2f uv_1(1.0f,0.0f);
	const Vec2f uv12(0.5f,0.5f);
	const Vec2f uv_2(0.0f,1.0f);
	const Vec2f uv20(0.0f,0.5f);
	const Vec2f uvcc(1.0f/3.0f);
	subdivide(patches[0],depth-1, uv_0,uv01,uvcc,uv20, false,csubdiv[1],csubdiv[2],false);
	subdivide(patches[1],depth-1, uv_1,uv12,uvcc,uv01, false,csubdiv[2],csubdiv[0],false);
	subdivide(patches[2],depth-1, uv_2,uv20,uvcc,uv12, false,csubdiv[0],csubdiv[1],false);
      } 

#if 0
      /* parametrization for quads */
      else if (N == 4) {
	const Vec2f uv_0(0.0f,0.0f);
	const Vec2f uv01(0.5f,0.0f);
	const Vec2f uv_1(1.0f,0.0f);
	const Vec2f uv12(1.0f,0.5f);
	const Vec2f uv_2(1.0f,1.0f);
	const Vec2f uv23(0.5f,1.0f);
	const Vec2f uv_3(0.0f,1.0f);
	const Vec2f uv30(0.0f,0.5f);
	const Vec2f uvcc(0.5f,0.5f);
	subdivide(patches[0],depth-1, uv_0,uv01,uvcc,uv30, false,csubdiv[1],csubdiv[3],false);
	subdivide(patches[1],depth-1, uv_1,uv12,uvcc,uv01, false,csubdiv[2],csubdiv[0],false);
	subdivide(patches[2],depth-1, uv_2,uv23,uvcc,uv12, false,csubdiv[3],csubdiv[1],false);
	subdivide(patches[3],depth-1, uv_3,uv30,uvcc,uv23, false,csubdiv[0],csubdiv[2],false);
      } 
#endif

      /* parametrization for arbitrary polygons */
      else {
	for (size_t i=0; i<N; i++) 
	  subdivide(patches[i],depth-1,
		    Vec2f(float(i)+0.0f,0.0f),Vec2f(float(i)+0.0f,1.0f),Vec2f(float(i)+1.0f,1.0f),Vec2f(float(i)+1.0f,0.0f),
		    false,csubdiv[(i+1)%N],csubdiv[(i-1)%N],false);
      }
    }

    void subdivide(const CatmullClarkPatch& patch, int depth,
                   const Vec2f& uv_0, const Vec2f& uv_1, const Vec2f& uv_2, const Vec2f& uv_3,              // uv range
                   bool Tt, bool Tr, bool Tb, bool Tl)                 // tagged transition edges
    {
      if (patch.dicable() || (depth <= 0))
        return tessellate(patch,uv_0,uv_1,uv_2,uv_3,Tt,Tr,Tb,Tl);

      CatmullClarkPatch patches[4]; 
      patch.subdivide(patches);

      const bool noleaf = depth > 1;
      const bool subdivide0 = noleaf && !patches[0].dicable();
      const bool subdivide1 = noleaf && !patches[1].dicable();
      const bool subdivide2 = noleaf && !patches[2].dicable();
      const bool subdivide3 = noleaf && !patches[3].dicable();

      const Vec2f uv01 = 0.5f*(uv_0+uv_1);
      const Vec2f uv12 = 0.5f*(uv_1+uv_2);
      const Vec2f uv23 = 0.5f*(uv_2+uv_3);
      const Vec2f uv30 = 0.5f*(uv_3+uv_0);
      const Vec2f uvcc = 0.25f*(uv_0+uv_1+uv_2+uv_3);

      if (subdivide0) subdivide (patches[0],depth-1, uv_0,uv01,uvcc,uv30, false,false,false,false);
      else            tessellate(patches[0],         uv_0,uv01,uvcc,uv30, false,subdivide1,subdivide3,false);

      if (subdivide1) subdivide (patches[1],depth-1, uv01,uv_1,uv12,uvcc, false,false,false,false); 
      else            tessellate(patches[1],         uv01,uv_1,uv12,uvcc, false,false,subdivide2,subdivide0);
      
      if (subdivide2) subdivide (patches[2],depth-1, uvcc,uv12,uv_2,uv23, false,false,false,false); 
      else            tessellate(patches[2],         uvcc,uv12,uv_2,uv23, subdivide1,false,false,subdivide3);
      
      if (subdivide3) subdivide (patches[3],depth-1, uv30,uvcc,uv23,uv_3, false,false,false,false); 
      else            tessellate(patches[3],         uv30,uvcc,uv23,uv_3, subdivide0,subdivide2,false,false);
    }

    void tessellate(const CatmullClarkPatch& patch, 
                    const Vec2f& uv0, const Vec2f& uv1, const Vec2f& uv2, const Vec2f& uv3,  
                    bool Tt, bool Tr, bool Tb, bool Tl)
    {
      GregoryPatch patcheval; 
      patcheval.init(patch);

      const float l0 = patch.ring[0].edge_level;
      const float l1 = patch.ring[1].edge_level;
      const float l2 = patch.ring[2].edge_level;
      const float l3 = patch.ring[3].edge_level;
      const TessellationPattern pattern0(l0,Tt);
      const TessellationPattern pattern1(l1,Tr);
      const TessellationPattern pattern2(l2,Tb);
      const TessellationPattern pattern3(l3,Tl);
      const TessellationPattern pattern_x = pattern0.size() > pattern2.size() ? pattern0 : pattern2;
      const TessellationPattern pattern_y = pattern1.size() > pattern3.size() ? pattern1 : pattern3;
      const int nx = pattern_x.size();
      const int ny = pattern_y.size();
      
      static int id = 0; id+=0x726849272;

      //const float du = (u1-u0)*(1.0f/8.0f);
      //const float dv = (v1-v0)*(1.0f/8.0f);
      for (int y=0; y<ny; y+=8) 
      {
        for (int x=0; x<nx; x+=8) 
        {
          count++;
          if (prims_o == NULL) continue;
          QuadQuad4x4* leaf = (QuadQuad4x4*) alloc.malloc(sizeof(QuadQuad4x4),16);
          new (leaf) QuadQuad4x4(id,geomID,primID);
          const BBox3fa bounds = leaf->build(scene,patcheval,pattern0,pattern1,pattern2,pattern3,pattern_x,x,nx,pattern_y,y,ny,uv0,uv1,uv2,uv3);
          *prims_o = PrimRef(bounds,BVH4::encodeTypedLeaf(leaf,0));
          prims_o++;
        }
      }
    }
  };
}

