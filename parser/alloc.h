// Copyright 2026 FlowCompute LLC
//
// This file is part of FlowCompute.
//
// FlowCompute is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// FlowCompute is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with FlowCompute. If not, see <https://www.gnu.org/licenses/>.

#ifndef TREE_SITTER_ALLOC_H_
#define TREE_SITTER_ALLOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Allow clients to override allocation functions
#ifdef TREE_SITTER_REUSE_ALLOCATOR

extern void *(*ts_current_malloc)(size_t size);
extern void *(*ts_current_calloc)(size_t count, size_t size);
extern void *(*ts_current_realloc)(void *ptr, size_t size);
extern void (*ts_current_free)(void *ptr);

#ifndef ts_malloc
#define ts_malloc  ts_current_malloc
#endif
#ifndef ts_calloc
#define ts_calloc  ts_current_calloc
#endif
#ifndef ts_realloc
#define ts_realloc ts_current_realloc
#endif
#ifndef ts_free
#define ts_free    ts_current_free
#endif

#else

#ifndef ts_malloc
#define ts_malloc  malloc
#endif
#ifndef ts_calloc
#define ts_calloc  calloc
#endif
#ifndef ts_realloc
#define ts_realloc realloc
#endif
#ifndef ts_free
#define ts_free    free
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif // TREE_SITTER_ALLOC_H_
