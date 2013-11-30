/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2011      Zynga Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "CCTextureAtlasEx.h"

//According to some tests GL_TRIANGLE_STRIP is slower, MUCH slower. Probably I'm doing something very wrong

// implementation CCTextureAtlasEx

NS_CC_BEGIN

CCTextureAtlasEx::CCTextureAtlasEx()
    :m_pIndices(NULL)
    ,m_pTexture(NULL)
    ,m_pQuads(NULL)
{}

CCTextureAtlasEx::~CCTextureAtlasEx()
{
    CCLOGINFO("cocos2d: CCTextureAtlasEx deallocing %p.", this);

    CC_SAFE_FREE(m_pQuads);
    CC_SAFE_FREE(m_pIndices);

    CC_SAFE_RELEASE(m_pTexture);

#if CC_ENABLE_CACHE_TEXTURE_DATA
    CCNotificationCenter::sharedNotificationCenter()->removeObserver(this, EVENT_COME_TO_FOREGROUND);
#endif
}

unsigned int CCTextureAtlasEx::getTotalQuads()
{
    return m_uTotalQuads;
}

unsigned int CCTextureAtlasEx::getCapacity()
{
    return m_uCapacity;
}

CCTexture2D* CCTextureAtlasEx::getTexture()
{
    return m_pTexture;
}

void CCTextureAtlasEx::setTexture(CCTexture2D * var)
{
    CC_SAFE_RETAIN(var);
    CC_SAFE_RELEASE(m_pTexture);
    m_pTexture = var;
}

ccV3F_C4B_T2F_Quad* CCTextureAtlasEx::getQuads()
{
    //if someone accesses the quads directly, presume that changes will be made
    return m_pQuads;
}

void CCTextureAtlasEx::setQuads(ccV3F_C4B_T2F_Quad *var)
{
    m_pQuads = var;
}

// TextureAtlas - alloc & init

CCTextureAtlasEx * CCTextureAtlasEx::create(const char* file, unsigned int capacity)
{
    CCTextureAtlasEx * pTextureAtlas = new CCTextureAtlasEx();
    if(pTextureAtlas && pTextureAtlas->initWithFile(file, capacity))
    {
        pTextureAtlas->autorelease();
        return pTextureAtlas;
    }
    CC_SAFE_DELETE(pTextureAtlas);
    return NULL;
}

CCTextureAtlasEx * CCTextureAtlasEx::createWithTexture(CCTexture2D *texture, unsigned int capacity)
{
    CCTextureAtlasEx * pTextureAtlas = new CCTextureAtlasEx();
    if (pTextureAtlas && pTextureAtlas->initWithTexture(texture, capacity))
    {
        pTextureAtlas->autorelease();
        return pTextureAtlas;
    }
    CC_SAFE_DELETE(pTextureAtlas);
    return NULL;
}

bool CCTextureAtlasEx::initWithFile(const char * file, unsigned int capacity)
{
    // retained in property
    CCTexture2D *texture = CCTextureCache::sharedTextureCache()->addImage(file);

    if (texture)
    {
        return initWithTexture(texture, capacity);
    }
    else
    {
        CCLOG("cocos2d: Could not open file: %s", file);
        return false;
    }
}

bool CCTextureAtlasEx::initWithTexture(CCTexture2D *texture, unsigned int capacity)
{
//    CCAssert(texture != NULL, "texture should not be null");
    m_uCapacity = capacity;
    m_uTotalQuads = 0;

    // retained in property
    this->m_pTexture = texture;
    CC_SAFE_RETAIN(m_pTexture);

    // Re-initialization is not allowed
    CCAssert(m_pQuads == NULL && m_pIndices == NULL, "");

    m_pQuads = (ccV3F_C4B_T2F_Quad*)malloc( m_uCapacity * sizeof(ccV3F_C4B_T2F_Quad) );
    m_pIndices = (GLushort *)malloc( m_uCapacity * 6 * sizeof(GLushort) );
    
    if( ! ( m_pQuads && m_pIndices) && m_uCapacity > 0) 
    {
        //CCLOG("cocos2d: CCTextureAtlasEx: not enough memory");
        CC_SAFE_FREE(m_pQuads);
        CC_SAFE_FREE(m_pIndices);

        // release texture, should set it to null, because the destruction will
        // release it too. see cocos2d-x issue #484
        CC_SAFE_RELEASE_NULL(m_pTexture);
        return false;
    }

    memset( m_pQuads, 0, m_uCapacity * sizeof(ccV3F_C4B_T2F_Quad) );
    memset( m_pIndices, 0, m_uCapacity * 6 * sizeof(GLushort) );

#if CC_ENABLE_CACHE_TEXTURE_DATA
    // listen the event when app go to background
    CCNotificationCenter::sharedNotificationCenter()->addObserver(this,
                                                           callfuncO_selector(CCTextureAtlasEx::listenBackToForeground),
                                                           EVENT_COME_TO_FOREGROUND,
                                                           NULL);
#endif
    
    this->setupIndices();

    return true;
}

void CCTextureAtlasEx::listenBackToForeground(CCObject *obj)
{
}

const char* CCTextureAtlasEx::description()
{
    return CCString::createWithFormat("<CCTextureAtlasEx | totalQuads = %u>", m_uTotalQuads)->getCString();
}


void CCTextureAtlasEx::setupIndices()
{
    if (m_uCapacity == 0)
        return;

    for( unsigned int i=0; i < m_uCapacity; i++)
    {
#if CC_TEXTURE_ATLAS_USE_TRIANGLE_STRIP
        m_pIndices[i*6+0] = i*4+0;
        m_pIndices[i*6+1] = i*4+0;
        m_pIndices[i*6+2] = i*4+2;        
        m_pIndices[i*6+3] = i*4+1;
        m_pIndices[i*6+4] = i*4+3;
        m_pIndices[i*6+5] = i*4+3;
#else
        m_pIndices[i*6+0] = i*4+0;
        m_pIndices[i*6+1] = i*4+1;
        m_pIndices[i*6+2] = i*4+2;

        // inverted index. issue #179
        m_pIndices[i*6+3] = i*4+3;
        m_pIndices[i*6+4] = i*4+2;
        m_pIndices[i*6+5] = i*4+1;        
#endif    
    }
}

// TextureAtlas - Update, Insert, Move & Remove

void CCTextureAtlasEx::updateQuad(ccV3F_C4B_T2F_Quad *quad, unsigned int index)
{
    CCAssert( index >= 0 && index < m_uCapacity, "updateQuadWithTexture: Invalid index");

    m_uTotalQuads = MAX( index+1, m_uTotalQuads);

    m_pQuads[index] = *quad;

}

void CCTextureAtlasEx::insertQuad(ccV3F_C4B_T2F_Quad *quad, unsigned int index)
{
    CCAssert( index < m_uCapacity, "insertQuadWithTexture: Invalid index");

    m_uTotalQuads++;
    CCAssert( m_uTotalQuads <= m_uCapacity, "invalid totalQuads");

    // issue #575. index can be > totalQuads
    unsigned int remaining = (m_uTotalQuads-1) - index;

    // last object doesn't need to be moved
    if( remaining > 0) 
    {
        // texture coordinates
        memmove( &m_pQuads[index+1],&m_pQuads[index], sizeof(m_pQuads[0]) * remaining );        
    }

    m_pQuads[index] = *quad;

}

void CCTextureAtlasEx::insertQuads(ccV3F_C4B_T2F_Quad* quads, unsigned int index, unsigned int amount)
{
    CCAssert(index + amount <= m_uCapacity, "insertQuadWithTexture: Invalid index + amount");

    m_uTotalQuads += amount;

    CCAssert( m_uTotalQuads <= m_uCapacity, "invalid totalQuads");

    // issue #575. index can be > totalQuads
    int remaining = (m_uTotalQuads-1) - index - amount;

    // last object doesn't need to be moved
    if( remaining > 0)
    {
        // tex coordinates
        memmove( &m_pQuads[index+amount],&m_pQuads[index], sizeof(m_pQuads[0]) * remaining );
    }


    unsigned int max = index + amount;
    unsigned int j = 0;
    for (unsigned int i = index; i < max ; i++)
    {
        m_pQuads[index] = quads[j];
        index++;
        j++;
    }
}

void CCTextureAtlasEx::insertQuadFromIndex(unsigned int oldIndex, unsigned int newIndex)
{
    CCAssert( newIndex >= 0 && newIndex < m_uTotalQuads, "insertQuadFromIndex:atIndex: Invalid index");
    CCAssert( oldIndex >= 0 && oldIndex < m_uTotalQuads, "insertQuadFromIndex:atIndex: Invalid index");

    if( oldIndex == newIndex )
    {
        return;
    }
    // because it is ambiguous in iphone, so we implement abs ourselves
    // unsigned int howMany = abs( oldIndex - newIndex);
    unsigned int howMany = (oldIndex - newIndex) > 0 ? (oldIndex - newIndex) :  (newIndex - oldIndex);
    unsigned int dst = oldIndex;
    unsigned int src = oldIndex + 1;
    if( oldIndex > newIndex)
    {
        dst = newIndex+1;
        src = newIndex;
    }

    // texture coordinates
    ccV3F_C4B_T2F_Quad quadsBackup = m_pQuads[oldIndex];
    memmove( &m_pQuads[dst],&m_pQuads[src], sizeof(m_pQuads[0]) * howMany );
    m_pQuads[newIndex] = quadsBackup;
}

void CCTextureAtlasEx::removeQuadAtIndex(unsigned int index)
{
    CCAssert( index < m_uTotalQuads, "removeQuadAtIndex: Invalid index");

    unsigned int remaining = (m_uTotalQuads-1) - index;


    // last object doesn't need to be moved
    if( remaining ) 
    {
        // texture coordinates
        memmove( &m_pQuads[index],&m_pQuads[index+1], sizeof(m_pQuads[0]) * remaining );
    }

    m_uTotalQuads--;
}

void CCTextureAtlasEx::removeQuadsAtIndex(unsigned int index, unsigned int amount)
{
    CCAssert(index + amount <= m_uTotalQuads, "removeQuadAtIndex: index + amount out of bounds");

    unsigned int remaining = (m_uTotalQuads) - (index + amount);

    m_uTotalQuads -= amount;

    if ( remaining )
    {
        memmove( &m_pQuads[index], &m_pQuads[index+amount], sizeof(m_pQuads[0]) * remaining );
    }
}

void CCTextureAtlasEx::removeAllQuads()
{
    m_uTotalQuads = 0;
}

// TextureAtlas - Resize
bool CCTextureAtlasEx::resizeCapacity(unsigned int newCapacity)
{
    if( newCapacity == m_uCapacity )
    {
        return true;
    }
    unsigned int uOldCapactiy = m_uCapacity; 
    // update capacity and totolQuads
    m_uTotalQuads = MIN(m_uTotalQuads, newCapacity);
    m_uCapacity = newCapacity;

    ccV3F_C4B_T2F_Quad* tmpQuads = NULL;
    GLushort* tmpIndices = NULL;
    
    // when calling initWithTexture(fileName, 0) on bada device, calloc(0, 1) will fail and return NULL,
    // so here must judge whether m_pQuads and m_pIndices is NULL.
    if (m_pQuads == NULL)
    {
        tmpQuads = (ccV3F_C4B_T2F_Quad*)malloc( m_uCapacity * sizeof(m_pQuads[0]) );
        if (tmpQuads != NULL)
        {
            memset(tmpQuads, 0, m_uCapacity * sizeof(m_pQuads[0]) );
        }
    }
    else
    {
        tmpQuads = (ccV3F_C4B_T2F_Quad*)realloc( m_pQuads, sizeof(m_pQuads[0]) * m_uCapacity );
        if (tmpQuads != NULL && m_uCapacity > uOldCapactiy)
        {
            memset(tmpQuads+uOldCapactiy, 0, (m_uCapacity - uOldCapactiy)*sizeof(m_pQuads[0]) );
        }
    }

    if (m_pIndices == NULL)
    {    
        tmpIndices = (GLushort*)malloc( m_uCapacity * 6 * sizeof(m_pIndices[0]) );
        if (tmpIndices != NULL)
        {
            memset( tmpIndices, 0, m_uCapacity * 6 * sizeof(m_pIndices[0]) );
        }
        
    }
    else
    {
        tmpIndices = (GLushort*)realloc( m_pIndices, sizeof(m_pIndices[0]) * m_uCapacity * 6 );
        if (tmpIndices != NULL && m_uCapacity > uOldCapactiy)
        {
            memset( tmpIndices+uOldCapactiy, 0, (m_uCapacity-uOldCapactiy) * 6 * sizeof(m_pIndices[0]) );
        }
    }

    if( ! ( tmpQuads && tmpIndices) ) {
        CCLOG("cocos2d: CCTextureAtlasEx: not enough memory");
        CC_SAFE_FREE(tmpQuads);
        CC_SAFE_FREE(tmpIndices);
        CC_SAFE_FREE(m_pQuads);
        CC_SAFE_FREE(m_pIndices);
        m_uCapacity = m_uTotalQuads = 0;
        return false;
    }

    m_pQuads = tmpQuads;
    m_pIndices = tmpIndices;


    setupIndices();

    return true;
}

void CCTextureAtlasEx::increaseTotalQuadsWith(unsigned int amount)
{
    m_uTotalQuads += amount;
}

void CCTextureAtlasEx::moveQuadsFromIndex(unsigned int oldIndex, unsigned int amount, unsigned int newIndex)
{
    CCAssert(newIndex + amount <= m_uTotalQuads, "insertQuadFromIndex:atIndex: Invalid index");
    CCAssert(oldIndex < m_uTotalQuads, "insertQuadFromIndex:atIndex: Invalid index");

    if( oldIndex == newIndex )
    {
        return;
    }
    //create buffer
    size_t quadSize = sizeof(ccV3F_C4B_T2F_Quad);
    ccV3F_C4B_T2F_Quad* tempQuads = (ccV3F_C4B_T2F_Quad*)malloc( quadSize * amount);
    memcpy( tempQuads, &m_pQuads[oldIndex], quadSize * amount );

    if (newIndex < oldIndex)
    {
        // move quads from newIndex to newIndex + amount to make room for buffer
        memmove( &m_pQuads[newIndex], &m_pQuads[newIndex+amount], (oldIndex-newIndex)*quadSize);
    }
    else
    {
        // move quads above back
        memmove( &m_pQuads[oldIndex], &m_pQuads[oldIndex+amount], (newIndex-oldIndex)*quadSize);
    }
    memcpy( &m_pQuads[newIndex], tempQuads, amount*quadSize);

    free(tempQuads);
}

void CCTextureAtlasEx::moveQuadsFromIndex(unsigned int index, unsigned int newIndex)
{
    CCAssert(newIndex + (m_uTotalQuads - index) <= m_uCapacity, "moveQuadsFromIndex move is out of bounds");

    memmove(m_pQuads + newIndex,m_pQuads + index, (m_uTotalQuads - index) * sizeof(m_pQuads[0]));
}

void CCTextureAtlasEx::fillWithEmptyQuadsFromIndex(unsigned int index, unsigned int amount)
{
    ccV3F_C4B_T2F_Quad quad;
    memset(&quad, 0, sizeof(quad));

    unsigned int to = index + amount;
    for (unsigned int i = index ; i < to ; i++)
    {
        m_pQuads[i] = quad;
    }
}

// TextureAtlas - Drawing

void CCTextureAtlasEx::drawQuads()
{
    this->drawNumberOfQuads(m_uTotalQuads, 0);
}

void CCTextureAtlasEx::drawNumberOfQuads(unsigned int n)
{
    this->drawNumberOfQuads(n, 0);
}

void CCTextureAtlasEx::drawNumberOfQuads(unsigned int n, unsigned int start)
{    
    if (0 == n) 
    {
        return;
    }
    ccGLBindTexture2D(m_pTexture->getName());
	
	#define kQuadSize sizeof(m_pQuads[0].bl)
    ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);

    // vertices
	long offset = (long)m_pQuads;
	int diff = offsetof( ccV3F_C4B_T2F, vertices);
    glVertexAttribPointer(kCCVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE, kQuadSize, (GLvoid*)(offset + diff));

    // colors
	diff = offsetof(ccV3F_C4B_T2F, colors);
    glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, kQuadSize, (GLvoid*)(offset + diff));

    // tex coords
	diff = offsetof(ccV3F_C4B_T2F, texCoords);
    glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, kQuadSize, (GLvoid*)(offset + diff));

#if CC_TEXTURE_ATLAS_USE_TRIANGLE_STRIP
    glDrawElements(GL_TRIANGLE_STRIP, (GLsizei)n*6, GL_UNSIGNED_SHORT, (GLvoid*) (start*6 + m_pIndices));
#else
    glDrawElements(GL_TRIANGLES, (GLsizei)n*6, GL_UNSIGNED_SHORT, (GLvoid*) (start*6 + m_pIndices));
#endif // CC_TEXTURE_ATLAS_USE_TRIANGLE_STRIP

    CC_INCREMENT_GL_DRAWS(1);
    CHECK_GL_ERROR_DEBUG();
}


NS_CC_END

