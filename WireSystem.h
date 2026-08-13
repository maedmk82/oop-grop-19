#ifndef WIRESYSTEM_H
#define WIRESYSTEM_H

#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <memory>
#include <sstream>
#include <istream>
#include "Component.h"

struct WirePoint {
    float x = 0.0f;
    float y = 0.0f;
};

struct WireEndpoint {
    int componentId = -1;
    int pinIndex = -1;

    bool isConnected() const {
        return componentId >= 0 && pinIndex >= 0;
    }
};

struct Wire {
    int id = -1;
    std::vector<WirePoint> points;
    WireEndpoint start;
    WireEndpoint end;
    bool isSelected = false;
};

struct JunctionDot {
    float x = 0.0f;
    float y = 0.0f;
};

class WireSystem {
public:
    static constexpr float HIT_RADIUS = 8.0f;
    static constexpr float JUNCTION_RADIUS = 4.0f;

    std::vector<Wire> wires;
    std::vector<JunctionDot> junctions;
    int nextWireId = 1;

    bool isDrawing = false;
    Wire activeWire;
    WirePoint previewPoint;

    void Clear() {
        wires.clear();
        junctions.clear();
        activeWire = Wire{};
        isDrawing = false;
        nextWireId = 1;
    }

    void ClearSelection() {
        for (auto& wire : wires) wire.isSelected = false;
    }

    static float DistanceSquared(float x1, float y1, float x2, float y2) {
        const float dx = x1 - x2;
        const float dy = y1 - y2;
        return dx * dx + dy * dy;
    }

    static float DistancePointToSegment(float px, float py,
                                        float x1, float y1,
                                        float x2, float y2,
                                        float* outX = nullptr,
                                        float* outY = nullptr) {
        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float lenSq = dx * dx + dy * dy;

        float t = 0.0f;
        if (lenSq > 0.000001f) {
            t = ((px - x1) * dx + (py - y1) * dy) / lenSq;
            t = std::max(0.0f, std::min(1.0f, t));
        }

        const float qx = x1 + t * dx;
        const float qy = y1 + t * dy;

        if (outX) *outX = qx;
        if (outY) *outY = qy;
        return std::sqrt(DistanceSquared(px, py, qx, qy));
    }

    static float Snap(float value, int gridSpacing) {
        if (gridSpacing <= 0) return value;
        return std::round(value / static_cast<float>(gridSpacing)) * gridSpacing;
    }

    static WirePoint SnapPoint(float x, float y, int gridSpacing) {
        return { Snap(x, gridSpacing), Snap(y, gridSpacing) };
    }

    Component* FindComponentById(std::vector<std::unique_ptr<Component>>& components, int id) const {
        for (auto& c : components) {
            if (c && c->id == id) return c.get();
        }
        return nullptr;
    }

    const Component* FindComponentById(const std::vector<std::unique_ptr<Component>>& components, int id) const {
        for (const auto& c : components) {
            if (c && c->id == id) return c.get();
        }
        return nullptr;
    }

    bool GetEndpointPosition(const WireEndpoint& endpoint,
                             const std::vector<std::unique_ptr<Component>>& components,
                             float& x, float& y) const {
        if (!endpoint.isConnected()) return false;

        const Component* component = FindComponentById(components, endpoint.componentId);
        if (!component) return false;
        if (endpoint.pinIndex < 0 || endpoint.pinIndex >= static_cast<int>(component->pins.size())) return false;

        const auto pos = component->GetPinWorldPos(component->pins[endpoint.pinIndex]);
        x = pos.first;
        y = pos.second;
        return true;
    }

    bool FindPinAt(float x, float y,
                   const std::vector<std::unique_ptr<Component>>& components,
                   Component*& outComponent, int& outPinIndex,
                   float radius = HIT_RADIUS) const {
        outComponent = nullptr;
        outPinIndex = -1;

        float bestDistSq = radius * radius;

        for (auto& component : components) {
            if (!component) continue;

            for (int i = 0; i < static_cast<int>(component->pins.size()); ++i) {
                const auto pos = component->GetPinWorldPos(component->pins[i]);
                const float distSq = DistanceSquared(x, y, pos.first, pos.second);

                if (distSq <= bestDistSq) {
                    bestDistSq = distSq;
                    outComponent = component.get();
                    outPinIndex = i;
                }
            }
        }

        return outComponent != nullptr;
    }

    bool FindWireHit(float x, float y, int& outWireIndex,
                     WirePoint& outPoint, float radius = HIT_RADIUS) const {
        outWireIndex = -1;
        outPoint = {};

        float bestDistance = radius;

        for (int i = 0; i < static_cast<int>(wires.size()); ++i) {
            const auto& wire = wires[i];
            if (wire.points.size() < 2) continue;

            for (size_t p = 1; p < wire.points.size(); ++p) {
                float qx = 0.0f;
                float qy = 0.0f;
                const float d = DistancePointToSegment(
                    x, y,
                    wire.points[p - 1].x, wire.points[p - 1].y,
                    wire.points[p].x, wire.points[p].y,
                    &qx, &qy
                );

                if (d <= bestDistance) {
                    bestDistance = d;
                    outWireIndex = i;
                    outPoint = { qx, qy };
                }
            }
        }

        return outWireIndex >= 0;
    }

    static void AddOrthogonalPoint(std::vector<WirePoint>& points,
                                   WirePoint target,
                                   int gridSpacing) {
        if (points.empty()) {
            points.push_back(target);
            return;
        }

        WirePoint last = points.back();
        target = SnapPoint(target.x, target.y, gridSpacing);

        if (std::fabs(last.x - target.x) < 0.001f &&
            std::fabs(last.y - target.y) < 0.001f) {
            return;
        }

        // Prefer horizontal first, then vertical.
        if (std::fabs(last.x - target.x) > 0.001f &&
            std::fabs(last.y - target.y) > 0.001f) {
            points.push_back({ target.x, last.y });
        }

        points.push_back(target);
    }

    void StartFromPin(Component* component, int pinIndex,
                      const std::vector<std::unique_ptr<Component>>& components) {
        if (!component || pinIndex < 0 || pinIndex >= static_cast<int>(component->pins.size())) return;

        const auto pos = component->GetPinWorldPos(component->pins[pinIndex]);

        activeWire = Wire{};
        activeWire.id = -1;
        activeWire.start = { component->id, pinIndex };
        activeWire.end = {};
        activeWire.points.push_back({ pos.first, pos.second });
        previewPoint = { pos.first, pos.second };
        isDrawing = true;
    }

    void StartFromPoint(float x, float y, int gridSpacing,
                        int existingWireIndex = -1) {
        activeWire = Wire{};
        activeWire.id = -1;
        activeWire.start = {};
        activeWire.end = {};

        WirePoint start = SnapPoint(x, y, gridSpacing);

        if (existingWireIndex >= 0 && existingWireIndex < static_cast<int>(wires.size())) {
            float qx = 0.0f;
            float qy = 0.0f;
            const auto& wire = wires[existingWireIndex];
            float best = HIT_RADIUS;
            for (size_t i = 1; i < wire.points.size(); ++i) {
                const float d = DistancePointToSegment(
                    x, y,
                    wire.points[i - 1].x, wire.points[i - 1].y,
                    wire.points[i].x, wire.points[i].y,
                    &qx, &qy
                );
                if (d < best) {
                    best = d;
                    start = SnapPoint(qx, qy, gridSpacing);
                }
            }
            AddJunction(start.x, start.y);
        }

        activeWire.points.push_back(start);
        previewPoint = start;
        isDrawing = true;
    }

    void UpdatePreview(float x, float y, int gridSpacing) {
        if (!isDrawing) return;
        previewPoint = SnapPoint(x, y, gridSpacing);
    }

    bool FinishAtPin(Component* component, int pinIndex,
                     const std::vector<std::unique_ptr<Component>>& components,
                     int gridSpacing) {
        if (!isDrawing || !component || pinIndex < 0 ||
            pinIndex >= static_cast<int>(component->pins.size())) {
            return false;
        }

        const auto pos = component->GetPinWorldPos(component->pins[pinIndex]);
        WirePoint endPoint = { pos.first, pos.second };

        AddOrthogonalPoint(activeWire.points, endPoint, gridSpacing);

        if (activeWire.points.size() < 2) {
            Cancel();
            return false;
        }

        activeWire.end = { component->id, pinIndex };
        activeWire.id = nextWireId++;
        activeWire.isSelected = false;
        wires.push_back(activeWire);
        isDrawing = false;
        activeWire = Wire{};
        RebuildJunctions(components);
        return true;
    }

    bool FinishAtPoint(float x, float y, int gridSpacing,
                       const std::vector<std::unique_ptr<Component>>& components,
                       bool forceJunction = false) {
        if (!isDrawing) return false;

        WirePoint endPoint = SnapPoint(x, y, gridSpacing);
        AddOrthogonalPoint(activeWire.points, endPoint, gridSpacing);

        if (activeWire.points.size() < 2) {
            Cancel();
            return false;
        }

        if (forceJunction) AddJunction(endPoint.x, endPoint.y);

        activeWire.id = nextWireId++;
        wires.push_back(activeWire);
        isDrawing = false;
        activeWire = Wire{};
        RebuildJunctions(components);
        return true;
    }

    void Cancel() {
        isDrawing = false;
        activeWire = Wire{};
        previewPoint = {};
    }

    void AddJunction(float x, float y) {
        for (const auto& j : junctions) {
            if (DistanceSquared(j.x, j.y, x, y) <= JUNCTION_RADIUS * JUNCTION_RADIUS) {
                return;
            }
        }
        junctions.push_back({ x, y });
    }

    void RemoveJunctionNear(float x, float y) {
        junctions.erase(
            std::remove_if(junctions.begin(), junctions.end(),
                [x, y](const JunctionDot& j) {
                    return DistanceSquared(j.x, j.y, x, y) <= JUNCTION_RADIUS * JUNCTION_RADIUS;
                }),
            junctions.end()
        );
    }

    bool HasJunctionNear(float x, float y) const {
        for (const auto& j : junctions) {
            if (DistanceSquared(j.x, j.y, x, y) <= JUNCTION_RADIUS * JUNCTION_RADIUS) return true;
        }
        return false;
    }

    void RebuildJunctions(const std::vector<std::unique_ptr<Component>>& components) {
        // Explicit junctions are intentionally preserved only while they
        // still lie on at least one existing wire. Crossing wires remain
        // electrically separate unless the user explicitly creates a dot.
        std::vector<JunctionDot> preserved = junctions;
        junctions.clear();

        for (const auto& j : preserved) {
            bool onWire = false;
            for (const auto& wire : wires) {
                for (size_t i = 1; i < wire.points.size(); ++i) {
                    if (DistancePointToSegment(j.x, j.y,
                                               wire.points[i - 1].x, wire.points[i - 1].y,
                                               wire.points[i].x, wire.points[i].y) <= JUNCTION_RADIUS + 0.5f) {
                        onWire = true;
                        break;
                    }
                }
                if (onWire) break;
            }
            if (onWire) AddJunction(j.x, j.y);
        }

        // If two different wires terminate at exactly the same free point,
        // show a junction dot. Pin-to-pin connections do not need a dot.
        for (size_t i = 0; i < wires.size(); ++i) {
            for (size_t j = i + 1; j < wires.size(); ++j) {
                const auto& a = wires[i];
                const auto& b = wires[j];
                if (a.points.empty() || b.points.empty()) continue;

                const WirePoint aStart = a.points.front();
                const WirePoint aEnd = a.points.back();
                const WirePoint bStart = b.points.front();
                const WirePoint bEnd = b.points.back();

                const WirePoint candidatesA[] = { aStart, aEnd };
                const WirePoint candidatesB[] = { bStart, bEnd };

                for (const auto& pa : candidatesA) {
                    for (const auto& pb : candidatesB) {
                        if (DistanceSquared(pa.x, pa.y, pb.x, pb.y) <= 0.01f) {
                            const bool aPin =
                                (pa.x == aStart.x && pa.y == aStart.y && a.start.isConnected()) ||
                                (pa.x == aEnd.x && pa.y == aEnd.y && a.end.isConnected());
                            const bool bPin =
                                (pb.x == bStart.x && pb.y == bStart.y && b.start.isConnected()) ||
                                (pb.x == bEnd.x && pb.y == bEnd.y && b.end.isConnected());

                            if (!(aPin && bPin)) AddJunction(pa.x, pa.y);
                        }
                    }
                }
            }
        }

        (void)components;
    }

    void DeleteSelected() {
        wires.erase(
            std::remove_if(wires.begin(), wires.end(),
                [](const Wire& w) { return w.isSelected; }),
            wires.end()
        );
    }

    void DeleteWiresConnectedToComponent(int componentId) {
        wires.erase(
            std::remove_if(wires.begin(), wires.end(),
                [componentId](const Wire& w) {
                    return w.start.componentId == componentId ||
                           w.end.componentId == componentId;
                }),
            wires.end()
        );
    }

    void Draw(SDL_Renderer* renderer,
              const std::vector<std::unique_ptr<Component>>& components) const {
        SDL_SetRenderDrawColor(renderer, 40, 80, 220, 255);

        for (const auto& wire : wires) {
            if (wire.points.size() < 2) continue;

            if (wire.isSelected) {
                SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 40, 80, 220, 255);
            }

            for (size_t i = 1; i < wire.points.size(); ++i) {
                float x1 = wire.points[i - 1].x;
                float y1 = wire.points[i - 1].y;
                float x2 = wire.points[i].x;
                float y2 = wire.points[i].y;

                if (i == 1) {
                    float ex = 0.0f, ey = 0.0f;
                    if (GetEndpointPosition(wire.start, components, ex, ey)) {
                        x1 = ex;
                        y1 = ey;
                    }
                }

                if (i == wire.points.size() - 1) {
                    float ex = 0.0f, ey = 0.0f;
                    if (GetEndpointPosition(wire.end, components, ex, ey)) {
                        x2 = ex;
                        y2 = ey;
                    }
                }

                SDL_RenderLine(renderer, x1, y1, x2, y2);
            }
        }

        // Active wire preview.
        if (isDrawing && !activeWire.points.empty()) {
            SDL_SetRenderDrawColor(renderer, 30, 180, 80, 255);

            const WirePoint last = activeWire.points.back();
            if (std::fabs(last.x - previewPoint.x) > 0.001f ||
                std::fabs(last.y - previewPoint.y) > 0.001f) {
                if (std::fabs(last.x - previewPoint.x) > 0.001f &&
                    std::fabs(last.y - previewPoint.y) > 0.001f) {
                    SDL_RenderLine(renderer, last.x, last.y, previewPoint.x, last.y);
                    SDL_RenderLine(renderer, previewPoint.x, last.y, previewPoint.x, previewPoint.y);
                } else {
                    SDL_RenderLine(renderer, last.x, last.y, previewPoint.x, previewPoint.y);
                }
            }
        }

        // Junction dots.
        for (const auto& j : junctions) {
            SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
            DrawFilledCircle(renderer, j.x, j.y, 4.0f);
        }
    }

    static void DrawFilledCircle(SDL_Renderer* renderer, float cx, float cy, float radius) {
        const int r = static_cast<int>(std::ceil(radius));
        for (int dy = -r; dy <= r; ++dy) {
            const float remain = radius * radius - static_cast<float>(dy * dy);
            if (remain < 0.0f) continue;
            const int dx = static_cast<int>(std::sqrt(remain));
            SDL_RenderLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
        }
    }

    std::string Serialize() const {
        std::ostringstream ss;
        ss << "WIRE_SYSTEM_V1\n";
        ss << "NEXT " << nextWireId << "\n";
        ss << "WIRES " << wires.size() << "\n";

        for (const auto& wire : wires) {
            ss << "W " << wire.id << " "
               << wire.start.componentId << " " << wire.start.pinIndex << " "
               << wire.end.componentId << " " << wire.end.pinIndex << " "
               << wire.points.size() << "\n";

            for (const auto& p : wire.points) {
                ss << p.x << " " << p.y << "\n";
            }
        }

        ss << "JUNCTIONS " << junctions.size() << "\n";
        for (const auto& j : junctions) {
            ss << j.x << " " << j.y << "\n";
        }

        return ss.str();
    }

    bool Deserialize(std::istream& in) {
        Clear();

        std::string token;
        if (!(in >> token)) return false;
        if (token != "WIRE_SYSTEM_V1") return false;

        size_t wireCount = 0;
        size_t junctionCount = 0;

        while (in >> token) {
            if (token == "NEXT") {
                in >> nextWireId;
            } else if (token == "WIRES") {
                in >> wireCount;
                for (size_t i = 0; i < wireCount; ++i) {
                    Wire wire;
                    size_t pointCount = 0;
                    in >> token;
                    if (token != "W") return false;

                    in >> wire.id
                       >> wire.start.componentId >> wire.start.pinIndex
                       >> wire.end.componentId >> wire.end.pinIndex
                       >> pointCount;

                    wire.points.resize(pointCount);
                    for (auto& p : wire.points) in >> p.x >> p.y;
                    wires.push_back(wire);
                    if (wire.id >= nextWireId) nextWireId = wire.id + 1;
                }
            } else if (token == "JUNCTIONS") {
                in >> junctionCount;
                for (size_t i = 0; i < junctionCount; ++i) {
                    JunctionDot j;
                    in >> j.x >> j.y;
                    junctions.push_back(j);
                }
                return true;
            }
        }

        return true;
    }
};

#endif
