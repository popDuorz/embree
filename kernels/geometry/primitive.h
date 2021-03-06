// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "../common/default.h"
#include "../common/scene.h"
#include "../../common/simd/simd.h"
#include "../common/primref.h"
#include "../common/primref_mb.h"

namespace embree
{
  struct PrimitiveType
  {
    /*! constructs the primitive type */
    PrimitiveType (const std::string& name, size_t bytes, size_t blockSize) 
    : name(name), bytes(bytes), blockSize(blockSize) {} 

    /*! Returns the number of stored primitives in a block. */
    virtual size_t size(const char* This) const = 0;

  public:
    std::string name;       //!< name of this primitive type
    size_t bytes;           //!< number of bytes of the triangle data
    size_t blockSize;       //!< block size
  };
}
