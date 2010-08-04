/* Orx - Portable Game Engine
 *
 * Copyright (c) 2008-2010 Orx-Project
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *    1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 *    2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 *    3. This notice may not be removed or altered from any source
 *    distribution.
 */

/**
 * @file orxTexture.c
 * @date 07/12/2003
 * @author iarwain@orx-project.org
 *
 */


#include "display/orxTexture.h"

#include "display/orxDisplay.h"
#include "memory/orxMemory.h"
#include "object/orxStructure.h"
#include "utils/orxHashTable.h"


/** Module flags
 */
#define orxTEXTURE_KU32_STATIC_FLAG_NONE        0x00000000

#define orxTEXTURE_KU32_STATIC_FLAG_READY       0x00000001

#define orxTEXTURE_KU32_STATIC_MASK_ALL         0xFFFFFFFF

/** Defines
 */
#define orxTEXTURE_KU32_FLAG_NONE               0x00000000

#define orxTEXTURE_KU32_FLAG_BITMAP             0x00000010
#define orxTEXTURE_KU32_FLAG_EXTERNAL           0x00000020
#define orxTEXTURE_KU32_FLAG_INTERNAL           0x00000040
#define orxTEXTURE_KU32_FLAG_REF_COORD          0x00000100
#define orxTEXTURE_KU32_FLAG_SIZE               0x00000200

#define orxTEXTURE_KU32_MASK_ALL                0xFFFFFFFF

#define orxTEXTURE_KU32_TABLE_SIZE              128

#define orxTEXTURE_KZ_PIXEL                     "pixel"


/***************************************************************************
 * Structure declaration                                                   *
 ***************************************************************************/

/** Texture structure
 */
struct __orxTEXTURE_t
{
  orxSTRUCTURE  stStructure;                    /**< Public structure, first structure member : 16 */
  orxU32        u32Counter;                     /**< Self reference counter : 20 */
  orxSTRING     zDataName;                      /**< Associated bitmap name : 24 */
  orxFLOAT      fWidth;                         /**< Width : 28 */
  orxFLOAT      fHeight;                        /**< Height : 32 */
  orxHANDLE     hData;                          /**< Data : 36 */
};

/** Static structure
 */
typedef struct __orxTEXTURE_STATIC_t
{
  orxHASHTABLE *pstTable;                       /**< Bitmap hashtable : 4 */
  orxTEXTURE   *pstScreen;                      /**< Screen texture : 8 */
  orxTEXTURE   *pstPixel;                       /**< Pixel texture : 12 */
  orxU32        u32Flags;                       /**< Control flags : 16 */

} orxTEXTURE_STATIC;


/***************************************************************************
 * Module global variable                                                  *
 ***************************************************************************/

static orxTEXTURE_STATIC sstTexture;


/***************************************************************************
 * Private functions                                                       *
 ***************************************************************************/

/** Deletes all textures
 */
static orxINLINE void orxTexture_DeleteAll()
{
  orxTEXTURE *pstTexture;

  /* Gets first texture */
  pstTexture = orxTEXTURE(orxStructure_GetFirst(orxSTRUCTURE_ID_TEXTURE));

  /* Non empty? */
  while(pstTexture != orxNULL)
  {
    /* Deletes texture */
    orxTexture_Delete(pstTexture);

    /* Gets first texture */
    pstTexture = orxTEXTURE(orxStructure_GetFirst(orxSTRUCTURE_ID_TEXTURE));
  }

  return;
}

/** Finds a texture by name
 * @param[in]   _wDataName    Name of the texture to find
 * @return      orxTEXTURE / orxNULL
 */
static orxINLINE orxTEXTURE *orxTexture_FindByName(const orxSTRING _zDataName)
{
  register orxTEXTURE *pstTexture;

  /* Gets texture from hash table */
  pstTexture = (orxTEXTURE *)orxHashTable_Get(sstTexture.pstTable, orxString_ToCRC(_zDataName));

  /* Done! */
  return pstTexture;
}


/***************************************************************************
 * Public functions                                                        *
 ***************************************************************************/

/** Setups the texture module
 */
void orxFASTCALL orxTexture_Setup()
{
  /* Adds module dependencies */
  orxModule_AddDependency(orxMODULE_ID_TEXTURE, orxMODULE_ID_MEMORY);
  orxModule_AddDependency(orxMODULE_ID_TEXTURE, orxMODULE_ID_STRUCTURE);
  orxModule_AddDependency(orxMODULE_ID_TEXTURE, orxMODULE_ID_DISPLAY);

  return;
}

/** Inits the texture module
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTexture_Init()
{
  orxSTATUS eResult;

  /* Not already Initialized? */
  if(!(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY))
  {
    /* Cleans static controller */
    orxMemory_Zero(&sstTexture, sizeof(orxTEXTURE_STATIC));

    /* Registers structure type */
    eResult = orxSTRUCTURE_REGISTER(TEXTURE, orxSTRUCTURE_STORAGE_TYPE_LINKLIST, orxMEMORY_TYPE_MAIN, orxNULL);

    /* Success? */
    if(eResult != orxSTATUS_FAILURE)
    {
      /* Creates hash table */
      sstTexture.pstTable = orxHashTable_Create(orxTEXTURE_KU32_TABLE_SIZE, orxHASHTABLE_KU32_FLAG_NONE, orxMEMORY_TYPE_MAIN);

      /* Success? */
      if(sstTexture.pstTable != orxNULL)
      {
        /* Updates flags for screen texture creation */
        sstTexture.u32Flags = orxTEXTURE_KU32_STATIC_FLAG_READY;

        /* Creates screen & pixel textures */
        sstTexture.pstScreen  = orxTexture_Create();
        sstTexture.pstPixel   = orxTexture_Create();

        /* Valid? */
        if((sstTexture.pstScreen != orxNULL) && (sstTexture.pstPixel != orxNULL))
        {
          /* Links screen bitmap */
          eResult = orxTexture_LinkBitmap(sstTexture.pstScreen, orxDisplay_GetScreenBitmap(), orxTEXTURE_KZ_SCREEN_NAME);

          /* Success? */
          if(eResult != orxSTATUS_FAILURE)
          {
            orxBITMAP *pstBitmap;

            /* Creates empty bitmap */
            pstBitmap = orxDisplay_CreateBitmap(1, 1);

            /* Success? */
            if(pstBitmap != orxNULL)
            {
              orxU8 au8PixelBuffer[] = {0xFF, 0xFF, 0xFF, 0xFF};

              /* Sets it data */
              if(orxDisplay_SetBitmapData(pstBitmap, au8PixelBuffer, 4 * sizeof(orxU8)) != orxSTATUS_FAILURE)
              {
                /* Links it */
                if(orxTexture_LinkBitmap(sstTexture.pstPixel, pstBitmap, orxTEXTURE_KZ_PIXEL) != orxSTATUS_FAILURE)
                {
                  /* Updates its flag */
                  orxStructure_SetFlags(sstTexture.pstPixel, orxTEXTURE_KU32_FLAG_INTERNAL, orxTEXTURE_KU32_FLAG_NONE);
                }
                else
                {
                  /* Deletes bitmap */
                  orxDisplay_DeleteBitmap(pstBitmap);

                  /* Updates result */
                  eResult = orxSTATUS_FAILURE;
                }
              }
              else
              {
                /* Deletes bitmap */
                orxDisplay_DeleteBitmap(pstBitmap);

                /* Updates result */
                eResult = orxSTATUS_FAILURE;
              }
            }
            else
            {
              /* Updates result */
              eResult = orxSTATUS_FAILURE;
            }
          }
        }
        else
        {
          /* Updates result */
          eResult = orxSTATUS_FAILURE;
        }
      }
      else
      {
        /* Updates result */
        eResult = orxSTATUS_FAILURE;
      }
    }
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_DISPLAY, "Tried to initialize texture module when it was already initialized.");

    /* Already initialized */
    eResult = orxSTATUS_SUCCESS;
  }

  /* Not initialized? */
  if(eResult == orxSTATUS_FAILURE)
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_DISPLAY, "Initializing texture module failed.");

    /* Deletes screen & pixel textures */
    if(sstTexture.pstScreen != orxNULL)
    {
      orxTexture_Delete(sstTexture.pstScreen);
      sstTexture.pstScreen = orxNULL;
    }
    if(sstTexture.pstPixel != orxNULL)
    {
      orxTexture_Delete(sstTexture.pstPixel);
      sstTexture.pstPixel = orxNULL;
    }

    /* Deletes hash table */
    if(sstTexture.pstTable != orxNULL)
    {
      orxHashTable_Delete(sstTexture.pstTable);
      sstTexture.pstTable = orxNULL;
    }

    /* Updates Flags */
    sstTexture.u32Flags &= ~orxTEXTURE_KU32_STATIC_FLAG_READY;
  }

  /* Done! */
  return eResult;
}

/** Exits from the texture module
 */
void orxFASTCALL orxTexture_Exit()
{
  /* Initialized? */
  if(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY)
  {
    /* Deletes screen & pixel textures */
    orxTexture_Delete(sstTexture.pstScreen);
    orxTexture_Delete(sstTexture.pstPixel);

    /* Deletes texture list */
    orxTexture_DeleteAll();

    /* Deletes hash table */
    orxHashTable_Delete(sstTexture.pstTable);
    sstTexture.pstTable = orxNULL;

    /* Unregisters structure type */
    orxStructure_Unregister(orxSTRUCTURE_ID_TEXTURE);

    /* Updates flags */
    sstTexture.u32Flags &= ~orxTEXTURE_KU32_STATIC_FLAG_READY;
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_DISPLAY, "Tried to exit texture module when it wasn't initialized.");
  }

  return;
}

/** Creates an empty texture
 * @return      orxTEXTURE / orxNULL
 */
orxTEXTURE *orxFASTCALL orxTexture_Create()
{
  orxTEXTURE *pstTexture;

  /* Checks */
  orxASSERT(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY);

  /* Creates texture */
  pstTexture = orxTEXTURE(orxStructure_Create(orxSTRUCTURE_ID_TEXTURE));

  /* Created? */
  if(pstTexture != orxNULL)
  {
    /* !!! INIT? !!! */
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_DISPLAY, "Failed to create structure for texture.");
  }

  return pstTexture;
}

/** Creates a texture from a bitmap file
 * @param[in]   _zBitmapFileName  Name of the bitmap
 * @return      orxTEXTURE / orxNULL
 */
orxTEXTURE *orxFASTCALL orxTexture_CreateFromFile(const orxSTRING _zBitmapFileName)
{
  orxTEXTURE *pstTexture;

  /* Checks */
  orxASSERT(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY);
  orxASSERT(_zBitmapFileName != orxNULL);

  /* Search for a texture using this bitmap */
  pstTexture = orxTexture_FindByName(_zBitmapFileName);

  /* Found? */
  if(pstTexture != orxNULL)
  {
    /* Increases self reference counter */
    pstTexture->u32Counter++;
  }
  else
  {
    /* Creates an empty texture */
    pstTexture = orxTexture_Create();

    /* Valid? */
    if(pstTexture != orxNULL)
    {
      orxBITMAP *pstBitmap;

      /* Loads bitmap */
      pstBitmap = orxDisplay_LoadBitmap(_zBitmapFileName);

      /* Assigns given bitmap to it */
      if((pstBitmap != orxNULL)
      && (orxTexture_LinkBitmap(pstTexture, pstBitmap, _zBitmapFileName) != orxSTATUS_FAILURE))
      {
          /* Inits it */
        orxStructure_SetFlags(pstTexture, orxTEXTURE_KU32_FLAG_INTERNAL, orxTEXTURE_KU32_FLAG_NONE);
      }
      else
      {
        /* Logs message */
        orxDEBUG_PRINT(orxDEBUG_LEVEL_DISPLAY, "Failed to load bitmap and link it to texture.");

        /* Frees allocated texture */
        orxTexture_Delete(pstTexture);

        /* Not created */
        pstTexture = orxNULL;
      }
    }
    else
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_DISPLAY, "Invalid texture created.");
    }
  }

  return pstTexture;
}

/** Deletes a texture (and its referenced bitmap)
 * @param[in]   _pstTexture     Concerned texture
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTexture_Delete(orxTEXTURE *_pstTexture)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstTexture);

  /* Is the last self reference? */
  if(_pstTexture->u32Counter == 0)
  {
    /* Not referenced from outside? */
    if(orxStructure_GetRefCounter(_pstTexture) == 0)
    {
      /* Cleans bitmap reference */
      orxTexture_UnlinkBitmap(_pstTexture);

      /* Deletes structure */
      orxStructure_Delete(_pstTexture);
    }
    else
    {
      /* Logs message */
      orxDEBUG_PRINT(orxDEBUG_LEVEL_DISPLAY, "Tried to delete texture object when it was still referenced.");

      /* Referenced by others */
      eResult = orxSTATUS_FAILURE;
    }
  }
  else
  {
    /* Decreases self reference counter */
    _pstTexture->u32Counter--;
  }

  /* Done! */
  return eResult;
}

/** Links a bitmap
 * @param[in]   _pstTexture     Concerned texture
 * @param[in]   _pstBitmap      Bitmap to link
 * @param[in]   _zDataName      Name associated with the bitmap (usually filename)
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTexture_LinkBitmap(orxTEXTURE *_pstTexture, const orxBITMAP *_pstBitmap, const orxSTRING _zDataName)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstTexture);
  orxASSERT(_pstBitmap != orxNULL);

  /* Unlink previous bitmap if needed */
  orxTexture_UnlinkBitmap(_pstTexture);

  /* Has no texture now? */
  if(orxStructure_TestFlags(_pstTexture, orxTEXTURE_KU32_FLAG_BITMAP) == orxFALSE)
  {
    orxTEXTURE *pstTexture;

    /* Search for a texture using this bitmap */
    pstTexture = orxTexture_FindByName(_zDataName);

    /* Found? */
    if(pstTexture != orxNULL)
    {
      /* Checks */
      orxASSERT(_pstBitmap == (orxBITMAP *)pstTexture->hData);

      /* Updates flags */
      orxStructure_SetFlags(_pstTexture, orxStructure_GetFlags(pstTexture, orxTEXTURE_KU32_MASK_ALL) | orxTEXTURE_KU32_FLAG_EXTERNAL, orxTEXTURE_KU32_FLAG_NONE);

      /* References external texture */
      _pstTexture->hData      = (orxHANDLE)pstTexture;

      /* Copies size & TL corner */
      _pstTexture->fWidth       = pstTexture->fWidth;
      _pstTexture->fHeight      = pstTexture->fHeight;

      /* Updates external texture self referenced counter */
      pstTexture->u32Counter++;
    }
    else
    {
       /* Updates flags */
      orxStructure_SetFlags(_pstTexture, orxTEXTURE_KU32_FLAG_BITMAP | orxTEXTURE_KU32_FLAG_SIZE, orxTEXTURE_KU32_FLAG_NONE);

      /* References bitmap */
      _pstTexture->hData = (orxHANDLE)_pstBitmap;

      /* Gets bitmap size */
      orxDisplay_GetBitmapSize(_pstBitmap, &(_pstTexture->fWidth), &(_pstTexture->fHeight));
    }

    /* Updates texture name */
    _pstTexture->zDataName = orxString_Duplicate(_zDataName);

    /* Adds it to hash table */
    orxHashTable_Add(sstTexture.pstTable, orxString_ToCRC(_zDataName), _pstTexture);
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_DISPLAY, "Texture is already linked.");

    /* Already linked */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Unlinks (and deletes if not used anymore) a bitmap
 * @param[in]   _pstTexture     Concerned texture
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTexture_UnlinkBitmap(orxTEXTURE *_pstTexture)
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  /* Checks */
  orxASSERT(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstTexture);

  /* Has bitmap */
  if(orxStructure_TestFlags(_pstTexture, orxTEXTURE_KU32_FLAG_BITMAP) != orxFALSE)
  {
    /* External bitmap? */
    if(orxStructure_TestFlags(_pstTexture, orxTEXTURE_KU32_FLAG_EXTERNAL) != orxFALSE)
    {
      /* Updates flags */
      orxStructure_SetFlags(_pstTexture, orxTEXTURE_KU32_FLAG_NONE, (orxTEXTURE_KU32_FLAG_BITMAP | orxTEXTURE_KU32_FLAG_EXTERNAL | orxTEXTURE_KU32_FLAG_SIZE));

      /* Decreases external texture self reference counter */
      orxTEXTURE(_pstTexture->hData)->u32Counter--;

      /* Cleans data */
      _pstTexture->hData = orxHANDLE_UNDEFINED;
    }
    else
    {
      /* Internally handled? */
      if(orxStructure_TestFlags(_pstTexture, orxTEXTURE_KU32_FLAG_INTERNAL))
      {
        /* Deletes bitmap */
        orxDisplay_DeleteBitmap((orxBITMAP *)(_pstTexture->hData));
      }

      /* Updates flags */
      orxStructure_SetFlags(_pstTexture, orxTEXTURE_KU32_FLAG_NONE, (orxTEXTURE_KU32_FLAG_BITMAP | orxTEXTURE_KU32_FLAG_SIZE | orxTEXTURE_KU32_FLAG_INTERNAL));

      /* Cleans data */
      _pstTexture->hData = orxHANDLE_UNDEFINED;
    }

    /* Removes from hash table */
    orxHashTable_Remove(sstTexture.pstTable, orxString_ToCRC(_pstTexture->zDataName));

    /* Deletes name */
    orxString_Delete(_pstTexture->zDataName);
    _pstTexture->zDataName = orxNULL;
  }
  else
  {
    /* No bitmap to unlink from */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Gets texture bitmap
 * @param[in]   _pstTexture     Concerned texture
 * @return      orxBITMAP / orxNULL
 */
orxBITMAP *orxFASTCALL orxTexture_GetBitmap(const orxTEXTURE *_pstTexture)
{
  register orxBITMAP *pstBitmap = orxNULL;

  /* Checks */
  orxASSERT(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstTexture);

  /* Has bitmap? */
  if(orxStructure_TestFlags(_pstTexture, orxTEXTURE_KU32_FLAG_BITMAP) != orxFALSE)
  {
    /* External bitmap? */
    if(orxStructure_TestFlags(_pstTexture, orxTEXTURE_KU32_FLAG_EXTERNAL) != orxFALSE)
    {
        pstBitmap = (orxBITMAP *)(orxTEXTURE(_pstTexture->hData)->hData);
    }
    /* Internal bitmap */
    else
    {
        pstBitmap = (orxBITMAP *)_pstTexture->hData;
    }
  }

  /* Done! */
  return pstBitmap;
}

/** Gets texture size
 * @param[in]   _pstTexture     Concerned texture
 * @param[out]  _pfWidth        Texture's width
 * @param[out]  _pfHeight       Texture's height
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTexture_GetSize(const orxTEXTURE *_pstTexture, orxFLOAT *_pfWidth, orxFLOAT *_pfHeight)
{
  orxSTATUS eResult;

  /* Checks */
  orxASSERT(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstTexture);
  orxASSERT(_pfWidth != orxNULL);
  orxASSERT(_pfHeight != orxNULL);

  /* Is screen? */
  if(_pstTexture == sstTexture.pstScreen)
  {
    /* Updates its size (screen might have changed) */
    orxDisplay_GetBitmapSize(orxDisplay_GetScreenBitmap(), &(((orxTEXTURE *)_pstTexture)->fWidth), &(((orxTEXTURE *)_pstTexture)->fHeight));
  }

  /* Has size? */
  if(orxStructure_TestFlags(_pstTexture, orxTEXTURE_KU32_FLAG_SIZE) != orxFALSE)
  {
    /* Stores values */
    *_pfWidth   = _pstTexture->fWidth;
    *_pfHeight  = _pstTexture->fHeight;

    /* Updates result */
    eResult = orxSTATUS_SUCCESS;
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_DISPLAY, "Texture's width is not set.");

    /* No size */
    *_pfWidth = *_pfHeight = orx2F(-1.0f);

    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Gets texture name
 * @param[in]   _pstTexture   Concerned texture
 * @return      Texture name / orxNULL
 */
const orxSTRING orxFASTCALL orxTexture_GetName(const orxTEXTURE *_pstTexture)
{
  orxSTRING zResult = orxNULL;

  /* Checks */
  orxASSERT(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstTexture);

  /* Updates result */
  zResult = _pstTexture->zDataName;

  /* Done! */
  return zResult;
}

/** Sets texture color
 * @param[in]   _pstTexture     Concerned texture
 * @param[in]   _pstColor       Color to set
 * @return      orxSTATUS_SUCCESS / orxSTATUS_FAILURE
 */
orxSTATUS orxFASTCALL orxTexture_SetColor(orxTEXTURE *_pstTexture, const orxCOLOR *_pstColor)
{
  orxBITMAP  *pstBitmap;
  orxSTATUS   eResult;

  /* Checks */
  orxASSERT(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY);
  orxSTRUCTURE_ASSERT(_pstTexture);
  orxASSERT(_pstColor != orxNULL);

  /* Gets bitmap */
  pstBitmap = orxTexture_GetBitmap(_pstTexture);

  /* Valid? */
  if(pstBitmap != orxNULL)
  {
    /* Updates its color */
    eResult = orxDisplay_SetBitmapColor(pstBitmap, orxColor_ToRGBA(_pstColor));
  }
  else
  {
    /* Logs message */
    orxDEBUG_PRINT(orxDEBUG_LEVEL_DISPLAY, "Invalid bitmap in texture.");

    /* Updates result */
    eResult = orxSTATUS_FAILURE;
  }

  /* Done! */
  return eResult;
}

/** Gets screen texture
 * @return      Screen texture / orxNULL
 */
orxTEXTURE *orxFASTCALL orxTexture_GetScreenTexture()
{
  /* Checks */
  orxASSERT(sstTexture.u32Flags & orxTEXTURE_KU32_STATIC_FLAG_READY);

  /* Done! */
  return(sstTexture.pstScreen);
}
