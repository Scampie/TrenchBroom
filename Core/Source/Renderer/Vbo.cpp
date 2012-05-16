/*
 Copyright (C) 2010-2012 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Vbo.h"
#include <cassert>
#include <cstring>
#include <algorithm>

namespace TrenchBroom {
    namespace Renderer {
        void VboBlock::insertBetween(VboBlock* previousBlock, VboBlock* nextBlock) {
            if (previousBlock != NULL) previousBlock->next = this;
            previous = previousBlock;
            if (nextBlock != NULL) nextBlock->previous = this;
            next = nextBlock;
        }
        
        VboBlock::VboBlock(Vbo& vbo, int address, int capacity) : m_vbo(vbo), address(address), capacity(capacity), free(true), previous(NULL), next(NULL) {}

        int VboBlock::writeBuffer(const unsigned char* buffer, unsigned int offset, unsigned int length) {
            assert(offset >= 0 && offset + length <= capacity);
            memcpy(m_vbo.m_buffer + address + offset, buffer, length);
            return offset + length;
        }
        
        int VboBlock::writeByte(unsigned char b, unsigned int offset) {
            assert(offset >= 0 && offset < capacity);
            m_vbo.m_buffer[address + offset] = b;
            return offset + 1;
        }
        
        int VboBlock::writeFloat(float f, unsigned int offset) {
            assert(offset >= 0 && offset + sizeof(float) <= capacity);
            memcpy(m_vbo.m_buffer + address + offset, &f, sizeof(float));
            return offset + sizeof(float);
        }
        
        int VboBlock::writeColor(const Vec4f& color, unsigned int offset) {
            assert(offset >= 0 && offset + 4 <= capacity);
            offset = writeByte((unsigned char)(color.x * 0xFF), offset);
            offset = writeByte((unsigned char)(color.y * 0xFF), offset);
            offset = writeByte((unsigned char)(color.z * 0xFF), offset);
            offset = writeByte((unsigned char)(color.w * 0xFF), offset);
            return offset;
        }
        
        int VboBlock::writeVec(const Vec4f& vec, unsigned int offset) {
            assert(offset >= 0 && offset + sizeof(Vec4f) <= capacity);
            memcpy(m_vbo.m_buffer + address + offset, &vec, sizeof(Vec4f));
            return offset + sizeof(Vec4f);
        }
        
        int VboBlock::writeVec(const Vec3f& vec, unsigned int offset) {
            assert(offset >= 0 && offset + sizeof(Vec3f) <= capacity);
            memcpy(m_vbo.m_buffer + address + offset, &vec, sizeof(Vec3f));
            return offset + sizeof(Vec3f);
        }
        
        int VboBlock::writeVec(const Vec2f& vec, unsigned int offset) {
            assert(offset >= 0 && offset + sizeof(Vec2f) <= capacity);
            memcpy(m_vbo.m_buffer + address + offset, &vec, sizeof(Vec2f));
            return offset + sizeof(Vec2f);
        }

        void VboBlock::freeBlock() {
            m_vbo.freeBlock(*this);
        }
        
        int VboBlock::compare(unsigned int anAddress, unsigned int aCapacity) {
            if (capacity < aCapacity) return -1;
            if (capacity > aCapacity) return 1;
            if (address < anAddress) return -1;
            if (address > anAddress) return 1;
            return 0;
        }

        unsigned int Vbo::findFreeBlockInRange(unsigned int address, unsigned int capacity, unsigned int start, unsigned int length) {
            assert(length > 0);
            
            int pivot = start + length / 2;
            VboBlock* block = m_freeBlocks[pivot];

            int order = block->compare(address, capacity);
            if (order == 0)
                return pivot;
            if (length == 1) {
                if (order > 0)
                    return pivot;
                return pivot + 1;
            }
            
            if (order > 0)
                return findFreeBlockInRange(address, capacity, start, length / 2);
            return findFreeBlockInRange(address, capacity, pivot, (length + 1) / 2);
        }
        
        unsigned int Vbo::findFreeBlock(unsigned int address, unsigned int capacity) {
            if (m_freeBlocks.empty()) return 0;
            unsigned int index = findFreeBlockInRange(address, capacity, 0, static_cast<int>(m_freeBlocks.size()));
            assert(index == m_freeBlocks.size() || capacity < m_freeBlocks[index]->capacity || (capacity == m_freeBlocks[index]->capacity && address <= m_freeBlocks[index]->address));
            return index;
        }

        void Vbo::insertFreeBlock(VboBlock& block) {
            assert(block.free);
            unsigned int index = findFreeBlock(block.address, block.capacity);
            assert(index >= 0 && index <= m_freeBlocks.size());
            if (index < m_freeBlocks.size())
                m_freeBlocks.insert(m_freeBlocks.begin() + index, &block);
            else
                m_freeBlocks.push_back(&block);
#ifdef _DEBUG_VBO
            checkFreeBlocks();
#endif
        }
        
        void Vbo::removeFreeBlock(VboBlock& block) {
            assert(block.free);
            unsigned int index = findFreeBlock(block.address, block.capacity);
            assert(index < m_freeBlocks.size());
            assert(m_freeBlocks[index] == &block);
            m_freeBlocks.erase(m_freeBlocks.begin() + index);
#ifdef _DEBUG_VBO
            checkFreeBlocks();
#endif
        }
        
        void Vbo::resizeVbo(unsigned int newCapacity) {
            bool wasActive = m_active;
            bool wasMapped = m_mapped;
            
            unsigned char* temp = NULL;
            if (m_vboId != 0 && m_freeCapacity < m_totalCapacity) {
                if (!wasActive) activate();
                if (!wasMapped) map();
                temp = new unsigned char[m_totalCapacity];
                memcpy(temp, m_buffer, m_totalCapacity);
            }
            
            unsigned int addedCapacity = newCapacity - m_totalCapacity;
            m_freeCapacity = newCapacity - (m_totalCapacity - m_freeCapacity);
            m_totalCapacity = newCapacity;
            
            if (m_last->free) {
                resizeBlock(*m_last, m_last->capacity + addedCapacity);
            } else {
                VboBlock* block = new VboBlock(*this, m_last->address + m_last->capacity, addedCapacity);
                block->insertBetween(m_last, NULL);
                insertFreeBlock(*block);
            }
            
            if (m_vboId != 0) {
                if (m_mapped) unmap();
                if (m_active) deactivate();
                glDeleteBuffers(1, &m_vboId);
                m_vboId = 0;
            }
            
            if (temp != NULL) {
                if (!m_active) activate();
                if (!m_mapped) map();
                
                memcpy(m_buffer, temp, m_totalCapacity - addedCapacity);
                delete [] temp;
                temp = NULL;
                
                if (!wasMapped) unmap();
                if (!wasActive) deactivate();
            } else {
                if (wasActive && !m_active) activate();
                if (wasMapped && !m_mapped) map();
            }
            
#ifdef _DEBUG_VBO
            checkBlockChain();
            checkFreeBlocks();
#endif
        }
        
        void Vbo::resizeBlock(VboBlock& block, unsigned int newCapacity) {
            if (block.capacity == newCapacity) return;
            if (block.free) {
                removeFreeBlock(block);
                block.capacity = newCapacity;
                insertFreeBlock(block);
            }
        }
        
        VboBlock* Vbo::packBlock(VboBlock& block) {
            VboBlock* first = block.next;
            if (first == NULL) return NULL;
            
            VboBlock* previous = block.previous;
            VboBlock* last = first;
            unsigned int size = 0;
            unsigned int address = first->address;
            
            do {
                last->address -= block.capacity;
                size += last->capacity;
                previous = last;
                last = last->next;
            } while (last != NULL && !last->free);
            
            if (size <= block.capacity) {
                memcpy(m_buffer + block.address, m_buffer + address, size);
            } else {
                unsigned char* temp = new unsigned char[size];
                memcpy(temp, m_buffer + address, size);
                memcpy(m_buffer + block.address, temp, size);
                delete [] temp;
            }
            
            if (last != NULL) {
                last->address -= block.capacity;
                resizeBlock(*last, last->capacity + block.capacity);
            } else {
                VboBlock* newBlock = new VboBlock(*this, previous->address + previous->capacity, block.capacity);
                insertFreeBlock(*newBlock);
                newBlock->insertBetween(previous, NULL);
                m_last = newBlock;
            }
            
            if (m_first == &block) m_first = block.next;
            
            removeFreeBlock(block);
            if (block.previous != NULL) block.previous->next = block.next;
            if (block.next != NULL) block.next->previous = block.previous;
            delete &block;

            return last;
        }
        
        Vbo::Vbo(GLenum type, unsigned int capacity) : m_type(type), m_totalCapacity(capacity), m_freeCapacity(capacity), m_buffer(NULL), m_vboId(0), m_active(false), m_mapped(false) {
            m_first = new VboBlock(*this, 0, m_totalCapacity);
            m_last = m_first;
            m_freeBlocks.push_back(m_first);
#ifdef _DEBUG_VBO
            checkBlockChain();
            checkFreeBlocks();
#endif
        }
        
        Vbo::~Vbo() {
#ifdef _DEBUG_VBO
            checkBlockChain();
            checkFreeBlocks();
#endif
            if (m_mapped) unmap();
            if (m_active) deactivate();
            if (m_vboId != 0) glDeleteBuffers(1, &m_vboId);
            m_freeBlocks.clear();
            VboBlock* block = m_first;
            while (block != NULL) {
                VboBlock* next = block->next;
                delete block;
                block = next;
            }
        }
        
        void Vbo::activate() {
            assert(!m_active);
            
            if (m_vboId == 0) {
                glGenBuffers(1, &m_vboId);
                glBindBuffer(m_type, m_vboId);
                glBufferData(m_type, m_totalCapacity, NULL, GL_DYNAMIC_DRAW);
            } else {
                glBindBuffer(m_type, m_vboId);
            }
            
            assert(glGetError() == GL_NO_ERROR);
            m_active = true;
        }
        
        void Vbo::deactivate() {
            assert(m_active);
            
            glBindBuffer(m_type, 0);
            m_active = false;
        }
        
        void Vbo::map() {
            assert(m_active);
            assert(!m_mapped);
            
            m_buffer = (unsigned char *)glMapBuffer(m_type, GL_WRITE_ONLY);
            assert(m_buffer != NULL);
            assert(glGetError() == GL_NO_ERROR);
            m_mapped = true;
        }
        
        void Vbo::unmap() {
            assert(m_active);
            assert(m_mapped);
            
            glUnmapBuffer(m_type);
            assert(glGetError() == GL_NO_ERROR);
            m_buffer = NULL;
            m_mapped = false;
        }
        
        VboBlock& Vbo::allocBlock(unsigned int capacity) {
            assert(capacity > 0);
            
#ifdef _DEBUG_VBO
            checkBlockChain();
            checkFreeBlocks();
#endif

            if (capacity > m_freeCapacity) {
                resizeVbo(2 * m_totalCapacity);
                return allocBlock(capacity);
            }
            
            unsigned int index = findFreeBlock(0, capacity);
            if (index >= m_freeBlocks.size()) {
                resizeVbo(2 * m_totalCapacity);
                return allocBlock(capacity);
            }
            
            VboBlock* block = m_freeBlocks[index];
            m_freeBlocks.erase(m_freeBlocks.begin() + index);
            
            // split block
            if (capacity < block->capacity) {
                VboBlock* remainder = new VboBlock(*this, block->address + capacity, block->capacity - capacity);
                remainder->insertBetween(block, block->next);
                block->capacity = capacity;
                insertFreeBlock(*remainder);
                if (m_last == block) m_last = remainder;
            }
            
            m_freeCapacity -= block->capacity;
            block->free = false;

#ifdef _DEBUG_VBO
            checkBlockChain();
            checkFreeBlocks();
#endif

            return *block;
        }
        
        VboBlock& Vbo::freeBlock(VboBlock& block) {
#ifdef _DEBUG_VBO
            checkBlockChain();
            checkFreeBlocks();
#endif

            VboBlock* previous = block.previous;
            VboBlock* next = block.next;
            
            m_freeCapacity += block.capacity;
            block.free = true;
            
            if (previous != NULL && previous->free && next != NULL && next->free) {
                resizeBlock(*previous, previous->capacity + block.capacity + next->capacity);
                if (m_last == next) m_last = previous;
                removeFreeBlock(*next);
                previous->insertBetween(previous->previous, next->next);
                delete &block;
                delete next;
                return *previous;
            }
            
            if (previous != NULL && previous->free) {
                resizeBlock(*previous, previous->capacity + block.capacity);
                if (m_last == &block) m_last = previous;
                previous->insertBetween(previous->previous, next);
                delete &block;
                return *previous;
            }
            
            if (next != NULL && next->free) {
                if (m_last == next) m_last = &block;
                removeFreeBlock(*next);
                block.capacity += next->capacity;
                block.free = true;
                block.insertBetween(previous, next->next);
                insertFreeBlock(block);
                delete next;
                return block;
            }
            
            insertFreeBlock(block);

#ifdef _DEBUG_VBO
            checkBlockChain();
            checkFreeBlocks();
#endif

            return block;
        }
        
        void Vbo::pack() {
            assert(m_mapped);
            
#ifdef _DEBUG_VBO
            checkBlockChain();
            checkFreeBlocks();
#endif

            if (m_totalCapacity == m_freeCapacity || (m_last->free && m_last->capacity == m_freeCapacity)) return;
            
            // find first free block
            VboBlock* block = m_first;
            while (block != NULL && !block->free)
                block = block->next;
            while (block != NULL && block->next != NULL)
                block = packBlock(*block);

#ifdef _DEBUG_VBO
            checkBlockChain();
            checkFreeBlocks();
#endif
        }

        void Vbo::checkBlockChain() {
            VboBlock* block = m_first;
            VboBlock* previous = NULL;
            assert(block->previous == NULL);
            
            while (block != NULL) {
                assert(&block->m_vbo == this);
                previous = block;
                block = block->next;
                assert(block == NULL || block->previous == previous);
            }
            
            assert(previous == m_last);
        }
        
        void Vbo::checkFreeBlocks() {
            for (unsigned int i = 0; i < m_freeBlocks.size(); i++) {
                VboBlock* block = m_freeBlocks[i];
                assert(block->free);
                if (i < m_freeBlocks.size() - 1)
                    assert((block->capacity < m_freeBlocks[i + 1]->capacity) || 
                           (block->capacity == m_freeBlocks[i + 1]->capacity && block->address < m_freeBlocks[i + 1]->address));
            }
        }
        
        bool Vbo::ownsBlock(VboBlock& block) {
            return &block.m_vbo == this;
        }
    }
}