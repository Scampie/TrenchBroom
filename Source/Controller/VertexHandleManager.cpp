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

#include "VertexHandleManager.h"

#include "Renderer/LinesRenderer.h"
#include "Renderer/PointHandleRenderer.h"

namespace TrenchBroom {
    namespace Model {
        VertexHandleHit::VertexHandleHit(HitType::Type type, const Vec3f& hitPoint, float distance, const Vec3f& vertex) :
        Hit(type, hitPoint, distance),
        m_vertex(vertex) {
            assert(type == HitType::VertexHandleHit ||
                   type == HitType::EdgeHandleHit ||
                   type == HitType::FaceHandleHit);
        }

        bool VertexHandleHit::pickable(Filter& filter) const {
            return true;
        }
    }

    namespace Controller {
        void VertexHandleManager::createRenderers() {
            assert(m_selectedHandleRenderer == NULL);
            assert(m_unselectedVertexHandleRenderer == NULL);
            assert(m_unselectedEdgeHandleRenderer == NULL);
            assert(m_unselectedFaceHandleRenderer == NULL);
            assert(m_selectedEdgeRenderer == NULL);

            Preferences::PreferenceManager& prefs = Preferences::PreferenceManager::preferences();
            float handleRadius = prefs.getFloat(Preferences::HandleRadius);
            float scalingFactor = prefs.getFloat(Preferences::HandleScalingFactor);
            float maxDistance = prefs.getFloat(Preferences::MaximumHandleDistance);
            m_selectedHandleRenderer = Renderer::PointHandleRenderer::create(handleRadius, 2, scalingFactor, maxDistance);
            m_unselectedVertexHandleRenderer = Renderer::PointHandleRenderer::create(handleRadius, 2, scalingFactor, maxDistance);
            m_unselectedEdgeHandleRenderer = Renderer::PointHandleRenderer::create(handleRadius, 2, scalingFactor, maxDistance);
            m_unselectedFaceHandleRenderer = Renderer::PointHandleRenderer::create(handleRadius, 2, scalingFactor, maxDistance);
            m_selectedEdgeRenderer = new Renderer::LinesRenderer();
            
            m_renderStateValid = false;
            m_recreateRenderers = false;
        }
        
        void VertexHandleManager::destroyRenderers() {
            delete m_unselectedVertexHandleRenderer;
            m_unselectedVertexHandleRenderer = NULL;
            delete m_unselectedEdgeHandleRenderer;
            m_unselectedEdgeHandleRenderer = NULL;
            delete m_unselectedFaceHandleRenderer;
            m_unselectedFaceHandleRenderer = NULL;
            delete m_selectedHandleRenderer;
            m_selectedHandleRenderer = NULL;
            delete m_selectedEdgeRenderer;
            m_selectedEdgeRenderer = NULL;
        }

        VertexHandleManager::VertexHandleManager() :
        m_totalVertexCount(0),
        m_selectedVertexCount(0),
        m_totalEdgeCount(0),
        m_selectedEdgeCount(0),
        m_totalFaceCount(0),
        m_selectedFaceCount(0),
        m_selectedHandleRenderer(NULL),
        m_unselectedVertexHandleRenderer(NULL),
        m_unselectedEdgeHandleRenderer(NULL),
        m_unselectedFaceHandleRenderer(NULL),
        m_selectedEdgeRenderer(NULL),
        m_renderStateValid(false),
        m_recreateRenderers(true) {}
        
        const Model::BrushList& VertexHandleManager::brushes(const Vec3f& handlePosition) const {
            Model::VertexToBrushesMap::const_iterator mapIt = m_selectedVertexHandles.find(handlePosition);
            if (mapIt != m_selectedVertexHandles.end())
                return mapIt->second;
            mapIt = m_unselectedVertexHandles.find(handlePosition);
            if (mapIt != m_unselectedVertexHandles.end())
                return mapIt->second;
            return Model::EmptyBrushList;
        }

        const Model::EdgeList& VertexHandleManager::edges(const Vec3f& handlePosition) const {
            Model::VertexToEdgesMap::const_iterator mapIt = m_selectedEdgeHandles.find(handlePosition);
            if (mapIt != m_selectedEdgeHandles.end())
                return mapIt->second;
            mapIt = m_unselectedEdgeHandles.find(handlePosition);
            if (mapIt != m_unselectedEdgeHandles.end())
                return mapIt->second;
            return Model::EmptyEdgeList;
        }

        const Model::FaceList& VertexHandleManager::faces(const Vec3f& handlePosition) const {
            Model::VertexToFacesMap::const_iterator mapIt = m_selectedFaceHandles.find(handlePosition);
            if (mapIt != m_selectedFaceHandles.end())
                return mapIt->second;
            mapIt = m_unselectedFaceHandles.find(handlePosition);
            if (mapIt != m_unselectedFaceHandles.end())
                return mapIt->second;
            return Model::EmptyFaceList;
        }

        void VertexHandleManager::add(Model::Brush& brush) {
            const Model::VertexList& brushVertices = brush.vertices();
            Model::VertexList::const_iterator vIt, vEnd;
            for (vIt = brushVertices.begin(), vEnd = brushVertices.end(); vIt != vEnd; ++vIt) {
                const Model::Vertex& vertex = **vIt;
                Model::VertexToBrushesMap::iterator mapIt = m_selectedVertexHandles.find(vertex.position);
                if (mapIt != m_selectedVertexHandles.end()) {
                    mapIt->second.push_back(&brush);
                    m_selectedVertexCount++;
                } else {
                    m_unselectedVertexHandles[vertex.position].push_back(&brush);
                }
            }
            m_totalVertexCount += brushVertices.size();

            const Model::EdgeList& brushEdges = brush.edges();
            Model::EdgeList::const_iterator eIt, eEnd;
            for (eIt = brushEdges.begin(), eEnd = brushEdges.end(); eIt != eEnd; ++eIt) {
                Model::Edge& edge = **eIt;
                Vec3f position = edge.center();
                Model::VertexToEdgesMap::iterator mapIt = m_selectedEdgeHandles.find(position);
                if (mapIt != m_selectedEdgeHandles.end()) {
                    mapIt->second.push_back(&edge);
                    m_selectedEdgeCount++;
                } else {
                    m_unselectedEdgeHandles[position].push_back(&edge);
                }
            }
            m_totalEdgeCount+= brushEdges.size();

            const Model::FaceList& brushFaces = brush.faces();
            Model::FaceList::const_iterator fIt, fEnd;
            for (fIt = brushFaces.begin(), fEnd = brushFaces.end(); fIt != fEnd; ++fIt) {
                Model::Face& face = **fIt;
                Vec3f position = face.center();
                Model::VertexToFacesMap::iterator mapIt = m_selectedFaceHandles.find(position);
                if (mapIt != m_selectedFaceHandles.end()) {
                    mapIt->second.push_back(&face);
                    m_selectedFaceCount++;
                } else {
                    m_unselectedFaceHandles[position].push_back(&face);
                }
            }
            m_totalFaceCount += brushFaces.size();

            m_renderStateValid = false;
        }

        void VertexHandleManager::add(const Model::BrushList& brushes) {
            Model::BrushList::const_iterator it, end;
            for (it = brushes.begin(), end = brushes.end(); it != end; ++it)
                add(**it);
        }

        void VertexHandleManager::remove(Model::Brush& brush) {
            const Model::VertexList& brushVertices = brush.vertices();
            Model::VertexList::const_iterator vIt, vEnd;
            for (vIt = brushVertices.begin(), vEnd = brushVertices.end(); vIt != vEnd; ++vIt) {
                const Model::Vertex& vertex = **vIt;
                if (removeHandle(vertex.position, brush, m_selectedVertexHandles)) {
                    assert(m_selectedVertexCount > 0);
                    m_selectedVertexCount--;
                } else {
                    removeHandle(vertex.position, brush, m_unselectedVertexHandles);
                }
            }
            assert(m_totalVertexCount >= brushVertices.size());
            m_totalVertexCount -= brushVertices.size();

            const Model::EdgeList& brushEdges = brush.edges();
            Model::EdgeList::const_iterator eIt, eEnd;
            for (eIt = brushEdges.begin(), eEnd = brushEdges.end(); eIt != eEnd; ++eIt) {
                Model::Edge& edge = **eIt;
                Vec3f position = edge.center();
                if (removeHandle(position, edge, m_selectedEdgeHandles)) {
                    assert(m_selectedEdgeCount > 0);
                    m_selectedEdgeCount--;
                } else {
                    removeHandle(position, edge, m_unselectedEdgeHandles);
                }
            }
            assert(m_totalEdgeCount >= brushEdges.size());
            m_totalEdgeCount -= brushEdges.size();

            const Model::FaceList& brushFaces = brush.faces();
            Model::FaceList::const_iterator fIt, fEnd;
            for (fIt = brushFaces.begin(), fEnd = brushFaces.end(); fIt != fEnd; ++fIt) {
                Model::Face& face = **fIt;
                Vec3f position = face.center();
                if (removeHandle(position, face, m_selectedFaceHandles)) {
                    assert(m_selectedFaceCount > 0);
                    m_selectedFaceCount--;
                } else {
                    removeHandle(position, face, m_unselectedFaceHandles);
                }
            }
            assert(m_totalFaceCount >= brushFaces.size());
            m_totalFaceCount -= brushFaces.size();

            m_renderStateValid = false;
        }

        void VertexHandleManager::remove(const Model::BrushList& brushes) {
            Model::BrushList::const_iterator it, end;
            for (it = brushes.begin(), end = brushes.end(); it != end; ++it)
                remove(**it);
        }

        void VertexHandleManager::clear() {
            m_unselectedVertexHandles.clear();
            m_selectedVertexHandles.clear();
            m_totalVertexCount = 0;
            m_selectedVertexCount = 0;
            m_unselectedEdgeHandles.clear();
            m_selectedEdgeHandles.clear();
            m_totalEdgeCount = 0;
            m_selectedEdgeCount = 0;
            m_unselectedFaceHandles.clear();
            m_selectedFaceHandles.clear();
            m_totalFaceCount = 0;
            m_selectedFaceCount = 0;
            m_renderStateValid = false;
        }

        void VertexHandleManager::selectVertexHandle(const Vec3f& position) {
            size_t count = 0;
            if ((count = moveHandle(position, m_unselectedVertexHandles, m_selectedVertexHandles)) > 0) {
                m_selectedVertexCount += count;
                m_renderStateValid = false;
            }
        }

        void VertexHandleManager::deselectVertexHandle(const Vec3f& position) {
            size_t count = 0;
            if ((count = moveHandle(position, m_selectedVertexHandles, m_unselectedVertexHandles)) > 0) {
                assert(m_selectedVertexCount >= count);
                m_selectedVertexCount -= count;
                m_renderStateValid = false;
            }
        }

        void VertexHandleManager::selectVertexHandles(const Vec3f::Set& positions) {
            Vec3f::Set::const_iterator it, end;
            for (it = positions.begin(), end = positions.end(); it != end; ++it)
                selectVertexHandle(*it);
        }

        void VertexHandleManager::deselectVertexHandles() {
            Model::VertexToBrushesMap::const_iterator vIt, vEnd;
            for (vIt = m_selectedVertexHandles.begin(), vEnd = m_selectedVertexHandles.end(); vIt != vEnd; ++vIt) {
                const Vec3f& position = vIt->first;
                const Model::BrushList& selectedBrushes = vIt->second;
                Model::BrushList& unselectedBrushes = m_unselectedVertexHandles[position];
                unselectedBrushes.insert(unselectedBrushes.begin(), selectedBrushes.begin(), selectedBrushes.end());
            }
            m_selectedVertexHandles.clear();
            m_selectedVertexCount = 0;
            m_renderStateValid = false;
        }

        void VertexHandleManager::selectEdgeHandle(const Vec3f& position) {
            size_t count = 0;
            if ((count = moveHandle(position, m_unselectedEdgeHandles, m_selectedEdgeHandles)) > 0) {
                m_selectedEdgeCount += count;
                m_renderStateValid = false;
            }
        }

        void VertexHandleManager::deselectEdgeHandle(const Vec3f& position) {
            size_t count = 0;
            if ((count = moveHandle(position, m_selectedEdgeHandles, m_unselectedEdgeHandles)) > 0) {
                assert(m_selectedEdgeCount >= count);
                m_selectedEdgeCount -= count;
                m_renderStateValid = false;
            }
        }

        void VertexHandleManager::selectEdgeHandles(const Model::EdgeInfoList& edges) {
            Model::EdgeInfoList::const_iterator it, end;
            for (it = edges.begin(), end = edges.end(); it != end; ++it) {
                const Model::EdgeInfo& edgeInfo = *it;
                selectEdgeHandle(edgeInfo.center());
            }
        }

        void VertexHandleManager::deselectEdgeHandles() {
            Model::VertexToEdgesMap::const_iterator eIt, eEnd;
            for (eIt = m_selectedEdgeHandles.begin(), eEnd = m_selectedEdgeHandles.end(); eIt != eEnd; ++eIt) {
                const Vec3f& position = eIt->first;
                const Model::EdgeList& selectedEdges = eIt->second;
                Model::EdgeList& unselectedEdges = m_unselectedEdgeHandles[position];
                unselectedEdges.insert(unselectedEdges.begin(), selectedEdges.begin(), selectedEdges.end());
            }
            m_selectedEdgeHandles.clear();
            m_selectedEdgeCount = 0;
            m_renderStateValid = false;
        }

        void VertexHandleManager::selectFaceHandle(const Vec3f& position) {
            size_t count = 0;
            if ((count = moveHandle(position, m_unselectedFaceHandles, m_selectedFaceHandles)) > 0) {
                m_selectedFaceCount += count;
                m_renderStateValid = false;
            }
        }

        void VertexHandleManager::deselectFaceHandle(const Vec3f& position) {
            size_t count = 0;
            if ((count = moveHandle(position, m_selectedFaceHandles, m_unselectedFaceHandles)) > 0) {
                assert(m_selectedFaceCount >= count);
                m_selectedFaceCount -= count;
                m_renderStateValid = false;
            }
        }

        void VertexHandleManager::selectFaceHandles(const Model::FaceInfoList& faces) {
            Model::FaceInfoList::const_iterator it, end;
            for (it = faces.begin(), end = faces.end(); it != end; ++it) {
                const Model::FaceInfo& faceInfo = *it;
                selectFaceHandle(faceInfo.center());
            }
        }

        void VertexHandleManager::deselectFaceHandles() {
            Model::VertexToFacesMap::const_iterator fIt, fEnd;
            for (fIt = m_selectedFaceHandles.begin(), fEnd = m_selectedFaceHandles.end(); fIt != fEnd; ++fIt) {
                const Vec3f& position = fIt->first;
                const Model::FaceList& selectedFaces = fIt->second;
                Model::FaceList& unselectedFaces = m_unselectedFaceHandles[position];
                unselectedFaces.insert(unselectedFaces.begin(), selectedFaces.begin(), selectedFaces.end());
            }
            m_selectedFaceHandles.clear();
            m_selectedFaceCount = 0;
            m_renderStateValid = false;
        }

        void VertexHandleManager::deselectAll() {
            deselectVertexHandles();
            deselectEdgeHandles();
            deselectFaceHandles();
        }

        void VertexHandleManager::pick(const Rayf& ray, Model::PickResult& pickResult, bool splitMode) const {
            Model::VertexToBrushesMap::const_iterator vIt, vEnd;
            Model::VertexToEdgesMap::const_iterator eIt, eEnd;
            Model::VertexToFacesMap::const_iterator fIt, fEnd;

            if ((m_selectedEdgeHandles.empty() && m_selectedFaceHandles.empty()) || splitMode) {
                for (vIt = m_unselectedVertexHandles.begin(), vEnd = m_unselectedVertexHandles.end(); vIt != vEnd; ++vIt) {
                    const Vec3f& position = vIt->first;
                    Model::VertexHandleHit* hit = pickHandle(ray, position, Model::HitType::VertexHandleHit);
                    if (hit != NULL)
                        pickResult.add(hit);
                }
            }

            for (vIt = m_selectedVertexHandles.begin(), vEnd = m_selectedVertexHandles.end(); vIt != vEnd; ++vIt) {
                const Vec3f& position = vIt->first;
                Model::VertexHandleHit* hit = pickHandle(ray, position, Model::HitType::VertexHandleHit);
                if (hit != NULL)
                    pickResult.add(hit);
            }

            if (m_selectedVertexHandles.empty() && m_selectedFaceHandles.empty() && !splitMode) {
                for (eIt = m_unselectedEdgeHandles.begin(), eEnd = m_unselectedEdgeHandles.end(); eIt != eEnd; ++eIt) {
                    const Vec3f& position = eIt->first;
                    Model::VertexHandleHit* hit = pickHandle(ray, position, Model::HitType::EdgeHandleHit);
                    if (hit != NULL)
                        pickResult.add(hit);
                }
            }

            for (eIt = m_selectedEdgeHandles.begin(), eEnd = m_selectedEdgeHandles.end(); eIt != eEnd; ++eIt) {
                const Vec3f& position = eIt->first;
                Model::VertexHandleHit* hit = pickHandle(ray, position, Model::HitType::EdgeHandleHit);
                if (hit != NULL)
                    pickResult.add(hit);
            }

            if (m_selectedVertexHandles.empty() && m_selectedEdgeHandles.empty() && !splitMode) {
                for (fIt = m_unselectedFaceHandles.begin(), fEnd = m_unselectedFaceHandles.end(); fIt != fEnd; ++fIt) {
                    const Vec3f& position = fIt->first;
                    Model::VertexHandleHit* hit = pickHandle(ray, position, Model::HitType::FaceHandleHit);
                    if (hit != NULL)
                        pickResult.add(hit);
                }
            }

            for (fIt = m_selectedFaceHandles.begin(), fEnd = m_selectedFaceHandles.end(); fIt != fEnd; ++fIt) {
                const Vec3f& position = fIt->first;
                Model::VertexHandleHit* hit = pickHandle(ray, position, Model::HitType::FaceHandleHit);
                if (hit != NULL)
                    pickResult.add(hit);
            }
        }

        void VertexHandleManager::render(Renderer::Vbo& vbo, Renderer::RenderContext& renderContext, bool splitMode) {
            if (!m_renderStateValid || m_recreateRenderers) {
                if (m_recreateRenderers) {
                    destroyRenderers();
                    createRenderers();
                }
                
                Model::VertexToBrushesMap::const_iterator vIt, vEnd;
                Model::VertexToEdgesMap::const_iterator eIt, eEnd;
                Model::VertexToFacesMap::const_iterator fIt, fEnd;

                m_unselectedVertexHandleRenderer->clear();
                m_unselectedEdgeHandleRenderer->clear();
                m_unselectedFaceHandleRenderer->clear();
                m_selectedHandleRenderer->clear();
                m_selectedEdgeRenderer->clear();

                if ((m_selectedEdgeHandles.empty() && m_selectedFaceHandles.empty()) || splitMode) {
                    for (vIt = m_unselectedVertexHandles.begin(), vEnd = m_unselectedVertexHandles.end(); vIt != vEnd; ++vIt) {
                        const Vec3f& position = vIt->first;
                        m_unselectedVertexHandleRenderer->add(position);
                    }
                }

                for (vIt = m_selectedVertexHandles.begin(), vEnd = m_selectedVertexHandles.end(); vIt != vEnd; ++vIt) {
                    const Vec3f& position = vIt->first;
                    m_selectedHandleRenderer->add(position);
                }

                if (m_selectedVertexHandles.empty() && m_selectedFaceHandles.empty() && !splitMode) {
                    for (eIt = m_unselectedEdgeHandles.begin(), eEnd = m_unselectedEdgeHandles.end(); eIt != eEnd; ++eIt) {
                        const Vec3f& position = eIt->first;
                        m_unselectedEdgeHandleRenderer->add(position);
                    }
                }

                for (eIt = m_selectedEdgeHandles.begin(), eEnd = m_selectedEdgeHandles.end(); eIt != eEnd; ++eIt) {
                    const Vec3f& position = eIt->first;
                    m_selectedHandleRenderer->add(position);
                    
                    const Model::EdgeList& edges = eIt->second;
                    Model::EdgeList::const_iterator edgeIt, edgeEnd;
                    for (edgeIt = edges.begin(), edgeEnd = edges.end(); edgeIt != edgeEnd; ++edgeIt) {
                        const Model::Edge& edge = **edgeIt;
                        m_selectedEdgeRenderer->add(edge.start->position, edge.end->position);
                    }
                }

                if (m_selectedVertexHandles.empty() && m_selectedEdgeHandles.empty() && !splitMode) {
                    for (fIt = m_unselectedFaceHandles.begin(), fEnd = m_unselectedFaceHandles.end(); fIt != fEnd; ++fIt) {
                        const Vec3f& position = fIt->first;
                        m_unselectedFaceHandleRenderer->add(position);
                    }
                }

                for (fIt = m_selectedFaceHandles.begin(), fEnd = m_selectedFaceHandles.end(); fIt != fEnd; ++fIt) {
                    const Vec3f& position = fIt->first;
                    m_selectedHandleRenderer->add(position);
                    
                    const Model::FaceList& faces = fIt->second;
                    Model::FaceList::const_iterator faceIt, faceEnd;
                    for (faceIt = faces.begin(), faceEnd = faces.end(); faceIt != faceEnd; ++faceIt) {
                        const Model::Face& face = **faceIt;
                        const Model::EdgeList& edges = face.edges();
                        Model::EdgeList::const_iterator edgeIt, edgeEnd;
                        for (edgeIt = edges.begin(), edgeEnd = edges.end(); edgeIt != edgeEnd; ++edgeIt) {
                            const Model::Edge& edge = **edgeIt;
                            m_selectedEdgeRenderer->add(edge.start->position, edge.end->position);
                        }
                    }
                }

                m_renderStateValid = true;
            }
            
            Preferences::PreferenceManager& prefs = Preferences::PreferenceManager::preferences();
            if (!m_selectedEdgeHandles.empty() || !m_selectedFaceHandles.empty()) {
                if (!m_selectedEdgeHandles.empty())
                    m_selectedEdgeRenderer->setColor(prefs.getColor(Preferences::EdgeHandleColor), prefs.getColor(Preferences::OccludedEdgeHandleColor));
                else
                    m_selectedEdgeRenderer->setColor(prefs.getColor(Preferences::FaceHandleColor), prefs.getColor(Preferences::OccludedFaceHandleColor));

                m_selectedEdgeRenderer->render(vbo, renderContext);
            }
            
            m_unselectedVertexHandleRenderer->setColor(prefs.getColor(Preferences::VertexHandleColor));
            m_unselectedEdgeHandleRenderer->setColor(prefs.getColor(Preferences::EdgeHandleColor));
            m_unselectedFaceHandleRenderer->setColor(prefs.getColor(Preferences::FaceHandleColor));
            if (splitMode)
                m_selectedHandleRenderer->setColor(prefs.getColor(Preferences::SelectedSplitHandleColor));
            else
                m_selectedHandleRenderer->setColor(prefs.getColor(Preferences::SelectedVertexHandleColor));

            m_unselectedVertexHandleRenderer->render(vbo, renderContext);
            m_unselectedEdgeHandleRenderer->render(vbo, renderContext);
            m_unselectedFaceHandleRenderer->render(vbo, renderContext);
            m_selectedHandleRenderer->render(vbo, renderContext);

            m_unselectedVertexHandleRenderer->setColor(prefs.getColor(Preferences::OccludedVertexHandleColor));
            m_unselectedEdgeHandleRenderer->setColor(prefs.getColor(Preferences::OccludedEdgeHandleColor));
            m_unselectedFaceHandleRenderer->setColor(prefs.getColor(Preferences::OccludedFaceHandleColor));
            if (splitMode)
                m_selectedHandleRenderer->setColor(prefs.getColor(Preferences::OccludedSelectedSplitHandleColor));
            else
                m_selectedHandleRenderer->setColor(prefs.getColor(Preferences::OccludedSelectedVertexHandleColor));

            glDisable(GL_DEPTH_TEST);
            m_unselectedVertexHandleRenderer->render(vbo, renderContext);
            m_unselectedEdgeHandleRenderer->render(vbo, renderContext);
            m_unselectedFaceHandleRenderer->render(vbo, renderContext);
            m_selectedHandleRenderer->render(vbo, renderContext);
            glEnable(GL_DEPTH_TEST);
        }
    }
}
