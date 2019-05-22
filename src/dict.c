/*
 * Copyright 2019 Yuta Hirokawa (University of Tsukuba, Japan)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define ZDICT_STATIC_LINKING_ONLY

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "./cbsl_internal.h"
#include <zdict.h>

/* FIXME: implicit declaration??? */
extern ZSTDLIB_API size_t ZSTD_CCtx_loadDictionary_byReference(ZSTD_CCtx* cctx, const void* dict, size_t dictSize);

static
cbsl_errors cbsl_train_data_finalize(void* p)
{
  CBSL_SAFE_FREE(p);
  return cbsl_error;
}

cbsl_errors cbsl_train_data(cbsl_ctx* ctx, const void* src, uint64_t sample_size, uint64_t total_size)
{
  CBSL_CHECK_COND_AND_RETURN(ctx != NULL, cbsl_error);
  CBSL_CHECK_COND_AND_RETURN(src != NULL, cbsl_error);
  CBSL_CHECK_COND_AND_RETURN(sample_size > 0, cbsl_error);
  CBSL_CHECK_COND_AND_RETURN(total_size > 0, cbsl_error);
  CBSL_CHECK_COND_AND_RETURN(ctx->mode == cbsl_store_mode, cbsl_error);
  CBSL_CHECK_COND_AND_RETURN(ctx->dictionary != NULL, cbsl_error);

  const size_t used_size = MIN(total_size, MAX_DICTIONARY_SIZE * 100);
  const size_t nsamples  = used_size / sample_size;

  size_t* sample_sizes = (size_t*)(malloc(sizeof(size_t) * nsamples));
  CBSL_CHECK_COND_AND_RETURN(sample_sizes != NULL, cbsl_error);
  for (size_t i = 0; i < nsamples; ++i)
  {
    sample_sizes[i] = sample_size;
  }

  const size_t dict_size_used = ZDICT_trainFromBuffer(ctx->dictionary, MAX_DICTIONARY_SIZE, src, sample_sizes, nsamples);
  CBSL_CHECK_COND_MSG_AND_RETURN(!ZDICT_isError(dict_size_used), ZDICT_getErrorName(dict_size_used), cbsl_train_data_finalize(sample_sizes));
  ctx->dict_size_used = dict_size_used;

  const size_t ret = ZSTD_CCtx_loadDictionary_byReference(ctx->zstd_cctx, ctx->dictionary, ctx->dict_size_used);
  CBSL_CHECK_COND_MSG_AND_RETURN(!ZSTD_isError(ret), ZSTD_getErrorName(ret), cbsl_train_data_finalize(sample_sizes));

  CBSL_SAFE_FREE(sample_sizes);
  return cbsl_success;
}
