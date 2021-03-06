#include <stdint.h>
#include <stdlib.h>

#include "OpenGL.h"
#include "FrameBuffer.h"
#include "RSP.h"
#include "RDP.h"
#include "Textures.h"
#include "ShaderCombiner.h"
#include "VI.h"

struct FrameBufferInfo frameBufferInfo;
CachedTexture *g_RDRAMtoFB;

void FrameBuffer_Init(void)
{
   frameBufferInfo.current = NULL;
   frameBufferInfo.top = NULL;
   frameBufferInfo.bottom = NULL;
   frameBufferInfo.numBuffers = 0;
}

void FrameBuffer_RemoveBottom(void)
{
   struct FrameBuffer *newBottom = (struct FrameBuffer*)
      frameBufferInfo.bottom->higher;

   TextureCache_Remove( frameBufferInfo.bottom->texture );

   if (frameBufferInfo.bottom == frameBufferInfo.top)
      frameBufferInfo.top = NULL;

   free( frameBufferInfo.bottom );

   frameBufferInfo.bottom = newBottom;

   if (frameBufferInfo.bottom != NULL)
      frameBufferInfo.bottom->lower = NULL;

   frameBufferInfo.numBuffers--;
}

void FrameBuffer_Remove( struct FrameBuffer *buffer )
{
   if ((buffer == frameBufferInfo.bottom) &&
         (buffer == frameBufferInfo.top))
   {
      frameBufferInfo.top = NULL;
      frameBufferInfo.bottom = NULL;
   }
   else if (buffer == frameBufferInfo.bottom)
   {
      frameBufferInfo.bottom = buffer->higher;

      if (frameBufferInfo.bottom)
         frameBufferInfo.bottom->lower = NULL;
   }
   else if (buffer == frameBufferInfo.top)
   {
      frameBufferInfo.top = buffer->lower;

      if (frameBufferInfo.top)
         frameBufferInfo.top->higher = NULL;
   }
   else
   {
      buffer->higher->lower = buffer->lower;
      buffer->lower->higher = buffer->higher;
   }

   if (buffer->texture)
      TextureCache_Remove( buffer->texture );

   free( buffer );

   frameBufferInfo.numBuffers--;
}

void FrameBuffer_RemoveBuffer( uint32_t address )
{
   struct FrameBuffer *current = (struct FrameBuffer*)frameBufferInfo.bottom;

   while (current != NULL)
   {
      if (current->m_startAddress == address)
      {
         current->texture = NULL;
         FrameBuffer_Remove( current );
         return;
      }
      current = current->higher;
   }
}

struct FrameBuffer *FrameBuffer_AddTop(void)
{
   struct FrameBuffer *newtop = (struct FrameBuffer*)malloc( sizeof(struct FrameBuffer ) );

   newtop->texture = TextureCache_AddTop();

   newtop->lower = frameBufferInfo.top;
   newtop->higher = NULL;

   if (frameBufferInfo.top)
      frameBufferInfo.top->higher = newtop;

   if (!frameBufferInfo.bottom)
      frameBufferInfo.bottom = newtop;

   frameBufferInfo.top = newtop;

   frameBufferInfo.numBuffers++;

   return newtop;
}

void FrameBuffer_MoveToTop( struct FrameBuffer *newtop )
{
   if (newtop == frameBufferInfo.top)
      return;

   if (newtop == frameBufferInfo.bottom)
   {
      frameBufferInfo.bottom = newtop->higher;
      frameBufferInfo.bottom->lower = NULL;
   }
   else
   {
      newtop->higher->lower = newtop->lower;
      newtop->lower->higher = newtop->higher;
   }

   newtop->higher = NULL;
   newtop->lower = frameBufferInfo.top;
   frameBufferInfo.top->higher = newtop;
   frameBufferInfo.top = newtop;

   TextureCache_MoveToTop( newtop->texture );
}

void FrameBuffer_Destroy(void)
{
   while (frameBufferInfo.bottom)
      FrameBuffer_RemoveBottom();
}

void FrameBuffer_SaveBuffer( uint32_t address, uint16_t format, uint16_t size, uint16_t width, uint16_t height, bool unknown)
{
   struct FrameBuffer *current = frameBufferInfo.top;

   if (width != VI.width && height == 0)
      return;

   (void)format;
   (void)unknown;

   /* Search through saved frame buffers */
   while (current != NULL)
   {
      if ((current->m_startAddress == address) &&
            (current->m_width == width) &&
            (current->m_height == height) &&
            (current->m_size == size))
      {
         if ((current->m_scaleX != OGL.scaleX) ||
               (current->m_scaleY != OGL.scaleY))
         {
            FrameBuffer_Remove( current );
            break;
         }

         /* code goes here */
         *(uint32_t*)&gfx_info.RDRAM[current->m_startAddress] = current->m_startAddress;

         current->m_changed = true;

         FrameBuffer_MoveToTop( current );

         gSP.changed |= CHANGED_TEXTURE;
         return;
      }
      current = current->lower;
   }

   /* Wasn't found, create a new one */
   current = FrameBuffer_AddTop();

   current->m_startAddress = address;
   current->m_endAddress = address + ((width * height << size >> 1) - 1);
   current->m_width = width;
   current->m_height = height;
   current->m_size = size;
   current->m_scaleX = OGL.scaleX;
   current->m_scaleY = OGL.scaleY;

   current->texture->width = current->m_width * OGL.scaleX;
   current->texture->height = current->m_height * OGL.scaleY;
   current->texture->clampS = 1;
   current->texture->clampT = 1;
   current->texture->address = current->m_startAddress;
   current->texture->clampWidth = current->m_width;
   current->texture->clampHeight = current->m_height;
   current->texture->frameBufferTexture = true;
   current->texture->maskS = 0;
   current->texture->maskT = 0;
   current->texture->mirrorS = 0;
   current->texture->mirrorT = 0;
   current->texture->realWidth = pow2( current->m_width * OGL.scaleX );
   current->texture->realHeight = pow2( current->m_height * OGL.scaleY );
   current->texture->textureBytes = current->texture->realWidth * current->texture->realHeight * 4;
   cache.cachedBytes += current->texture->textureBytes;

   /* code goes here - just bind texture and copy it over */
   *(uint32_t*)&gfx_info.RDRAM[current->m_startAddress] = current->m_startAddress;

   current->m_changed = true;

   gSP.changed |= CHANGED_TEXTURE;
}

struct FrameBuffer *FrameBuffer_GetCurrent(void)
{
   return (struct FrameBuffer*)frameBufferInfo.top;
}

void FrameBuffer_RenderBuffer( uint32_t address )
{
   struct FrameBuffer *current = (struct FrameBuffer*)frameBufferInfo.top;

   while (current != NULL)
   {
      if ((current->m_startAddress <= address) &&
            (current->m_endAddress >= address))
      {
         /* code goes here */

         current->m_changed = false;

         FrameBuffer_MoveToTop( current );

         gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
         gDP.changed |= CHANGED_COMBINE;
         return;
      }
      current = current->lower;
   }
}

void FrameBuffer_RestoreBuffer( uint32_t address, uint16_t size, uint16_t width )
{
   struct FrameBuffer *current = (struct FrameBuffer*)frameBufferInfo.top;

   while (current != NULL)
   {
      if ((current->m_startAddress == address) &&
            (current->m_width == width) &&
            (current->m_size == size))
      {

         /* code goes here */

         FrameBuffer_MoveToTop( current );

         gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
         gDP.changed |= CHANGED_COMBINE;
         return;
      }
      current = current->lower;
   }
}

struct FrameBuffer *FrameBuffer_FindBuffer( uint32_t address )
{
   struct FrameBuffer *current = (struct FrameBuffer*)frameBufferInfo.top;

   while (current)
   {
      if ((current->m_startAddress <= address) &&
            (current->m_endAddress >= address))
         return current;
      current = current->lower;
   }

   return NULL;
}

void FrameBuffer_ActivateBufferTexture( int16_t t, struct FrameBuffer *buffer )
{
   buffer->texture->scaleS = OGL.scaleX / (float)buffer->texture->realWidth;
   buffer->texture->scaleT = OGL.scaleY / (float)buffer->texture->realHeight;

   if (gSP.textureTile[t]->shifts > 10)
      buffer->texture->shiftScaleS = (float)(1 << (16 - gSP.textureTile[t]->shifts));
   else if (gSP.textureTile[t]->shifts > 0)
      buffer->texture->shiftScaleS = 1.0f / (float)(1 << gSP.textureTile[t]->shifts);
   else
      buffer->texture->shiftScaleS = 1.0f;

   if (gSP.textureTile[t]->shiftt > 10)
      buffer->texture->shiftScaleT = (float)(1 << (16 - gSP.textureTile[t]->shiftt));
   else if (gSP.textureTile[t]->shiftt > 0)
      buffer->texture->shiftScaleT = 1.0f / (float)(1 << gSP.textureTile[t]->shiftt);
   else
      buffer->texture->shiftScaleT = 1.0f;

   if (gDP.loadTile->loadType == LOADTYPE_TILE)
   {
      buffer->texture->offsetS = gDP.loadTile->uls;
      buffer->texture->offsetT = (float)buffer->m_height - 
         (gDP.loadTile->ult + (gDP.textureImage.address - buffer->m_startAddress) / (buffer->m_width << buffer->m_size >> 1));
   }
   else
   {
      buffer->texture->offsetS = 0.0f;
      buffer->texture->offsetT = (float)buffer->m_height - (gDP.textureImage.address - buffer->m_startAddress) / (buffer->m_width << buffer->m_size >> 1);
   }

   FrameBuffer_MoveToTop( buffer );
   TextureCache_ActivateTexture( t, buffer->texture );
	gDP.changed |= CHANGED_FB_TEXTURE;
}

void FrameBuffer_ActivateBufferTextureBG(int16_t t, struct FrameBuffer *buffer )
{
	if (buffer == NULL || buffer->texture == NULL)
		return;

   buffer->texture->scaleS = OGL.scaleX / (float)buffer->texture->realWidth;
   buffer->texture->scaleT = OGL.scaleY / (float)buffer->texture->realHeight;

	buffer->texture->shiftScaleS = 1.0f;
	buffer->texture->shiftScaleT = 1.0f;

	buffer->texture->offsetS = gSP.bgImage.imageX;
	buffer->texture->offsetT = (float)buffer->m_height - gSP.bgImage.imageY;

   FrameBuffer_MoveToTop( buffer );
   TextureCache_ActivateTexture( t, buffer->texture );
	gDP.changed |= CHANGED_FB_TEXTURE;
}

void FrameBuffer_CopyFromRDRAM( uint32_t _address, bool _bUseAlpha )
{
   /* stub */
}

void FrameBuffer_CopyToRDRAM( uint32_t _address )
{
   /* stub */
}

void FrameBuffer_CopyDepthBuffer( uint32_t _address )
{
   /* stub */
}
