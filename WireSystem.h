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
#include <functional>
#include <limits>
#include <unordered_map>
#include <unordered_set>
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
    std::vector<int> connectedWireIds;
};

struct ElectricalPinRef {
    int componentId = -1;
    int pinIndex = -1;

    bool operator==(const ElectricalPinRef& other) const {
        return componentId == other.componentId && pinIndex == other.pinIndex;
    }
};

struct ElectricalNet {
    int id = -1;
    std::vector<ElectricalPinRef> pins;
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

    std::function<void(const std::vector<ElectricalNet>&)> onElectricalTopologyChanged;

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

    // ====================================================================
    // تابعی که جا افتاده بود (برای رسم زنده سیم با زاویه 90 درجه)
    // ====================================================================
    // ====================================================================
    // کلیک‌های میانی در فضای خالی (همچنان روی گرید قفل می‌شوند اما بدون زیگ‌زاگ)
    // ====================================================================
    static void AddOrthogonalPoint(std::vector<WirePoint>& points,
                                   WirePoint target,
                                   int gridSpacing) {
        if (points.empty()) {
            points.push_back(SnapPoint(target.x, target.y, gridSpacing));
            return;
        }

        WirePoint last = points.back();
        target = SnapPoint(target.x, target.y, gridSpacing);

        if (std::fabs(last.x - target.x) < 0.001f && std::fabs(last.y - target.y) < 0.001f) {
            return;
        }

        if (std::fabs(last.x - target.x) > 0.001f && std::fabs(last.y - target.y) > 0.001f) {
            // حفظ جهت حرکت قبلی سیم برای جلوگیری از شکستگی
            if (points.size() >= 2) {
                WirePoint prev = points[points.size() - 2];
                if (std::fabs(prev.x - last.x) < 0.001f) {
                    points.push_back({ last.x, target.y });
                } else {
                    points.push_back({ target.x, last.y });
                }
            } else {
                points.push_back({ target.x, last.y });
            }
        }

        points.push_back(target);
        SimplifyPoints(points);
    }
    // ====================================================================

    // سیستم هوشمند ساده‌ساز سیم: حذف خطوط روی هم افتاده و نقاط تکراری
    static void SimplifyPoints(std::vector<WirePoint>& points) {
        if (points.size() < 2) return;

        bool changed = true;
        while (changed) {
            changed = false;

            // 1. پاک کردن نقاط تکراری که روی هم افتاده‌اند
            std::vector<WirePoint> dedup;
            for (const auto& p : points) {
                if (dedup.empty() ||
                    std::fabs(dedup.back().x - p.x) > 0.001f ||
                    std::fabs(dedup.back().y - p.y) > 0.001f) {
                    dedup.push_back(p);
                }
            }
            if (dedup.size() != points.size()) changed = true;
            points = std::move(dedup);

            // 2. حذف خطوط برگشتی (تا شده) و هم‌راستا
            if (points.size() >= 3) {
                std::vector<WirePoint> temp;
                temp.push_back(points.front());
                bool removedCollinear = false;

                for (size_t i = 1; i < points.size() - 1; ++i) {
                    const WirePoint& p1 = temp.back();
                    const WirePoint& p2 = points[i];
                    const WirePoint& p3 = points[i+1];

                    bool sameX = (std::fabs(p1.x - p2.x) < 0.001f && std::fabs(p2.x - p3.x) < 0.001f);
                    bool sameY = (std::fabs(p1.y - p2.y) < 0.001f && std::fabs(p2.y - p3.y) < 0.001f);

                    if (sameX || sameY) {
                        removedCollinear = true; // نقطه وسط (p2) را نادیده می‌گیریم تا سیم صاف شود
                    } else {
                        temp.push_back(p2);
                    }
                }
                temp.push_back(points.back());

                if (removedCollinear) {
                    points = std::move(temp);
                    changed = true;
                }
            }
        }
    }
    // ====================================================================

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
                   float radius = -1.0f) const {
        outComponent = nullptr;
        outPinIndex = -1;
        float bestDistSq = std::numeric_limits<float>::max();
        for (auto& component : components) {
            if (!component) continue;
            for (int i = 0; i < static_cast<int>(component->pins.size()); ++i) {
                const Pin& pin = component->pins[i];
                const auto pos = component->GetPinWorldPos(pin);
                const float hitRadius = (radius > 0.0f) ? radius : std::max(1.0f, pin.sensitivityRadius);
                const float distSq = DistanceSquared(x, y, pos.first, pos.second);
                if (distSq <= hitRadius * hitRadius && distSq < bestDistSq) {
                    bestDistSq = distSq;
                    outComponent = component.get();
                    outPinIndex = i;
                }
            }
        }
        return outComponent != nullptr;
    }

    void UpdatePinHighlights(
        const std::vector<std::unique_ptr<Component>>& components,
        float mouseX, float mouseY) const {
        for (const auto& component : components) {
            if (!component) continue;
            for (auto& pin : component->pins) {
                const auto pos = component->GetPinWorldPos(pin);
                pin.checkMouseOver(pos.first, pos.second, mouseX, mouseY);
            }
        }
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
                float qx = 0.0f, qy = 0.0f;
                const float d = DistancePointToSegment(
                    x, y, wire.points[p - 1].x, wire.points[p - 1].y,
                    wire.points[p].x, wire.points[p].y, &qx, &qy);
                if (d <= bestDistance) {
                    bestDistance = d;
                    outWireIndex = i;
                    outPoint = { qx, qy };
                }
            }
        }
        return outWireIndex >= 0;
    }

    static std::vector<WirePoint> BuildShortestOrthogonalPath(
        WirePoint start, WirePoint end, int gridSpacing) {
        start = SnapPoint(start.x, start.y, gridSpacing);
        end   = SnapPoint(end.x, end.y, gridSpacing);
        std::vector<WirePoint> path;
        path.push_back(start);
        if (std::fabs(start.x - end.x) < 0.001f && std::fabs(start.y - end.y) < 0.001f) {
            return path;
        }
        if (std::fabs(start.x - end.x) > 0.001f && std::fabs(start.y - end.y) > 0.001f) {
            path.push_back({ end.x, start.y });
        }
        path.push_back(end);
        return path;
    }

    static void AppendOrthogonalPath(
        std::vector<WirePoint>& points,
        WirePoint target,  // مختصات دقیق پین قطعه (بدون اسنپ اجباری)
        int gridSpacing) {

        if (points.empty()) {
            points.push_back(target);
            return;
        }

        WirePoint exactStart = points.back();

        // رسم مسیر ۹۰ درجه به صورت مستقیم
        if (std::fabs(exactStart.x - target.x) > 0.001f &&
            std::fabs(exactStart.y - target.y) > 0.001f) {

            // برای جلوگیری از پله شدن، جهت خط قبلی سیم را چک می‌کنیم
            if (points.size() >= 2) {
                WirePoint prev = points[points.size() - 2];
                if (std::fabs(prev.x - exactStart.x) < 0.001f) {
                    // قطعه قبلی عمودی بوده، پس اول مسیر عمودی را ادامه می‌دهیم
                    points.push_back({ exactStart.x, target.y });
                } else {
                    // قطعه قبلی افقی بوده، پس اول مسیر افقی را ادامه می‌دهیم
                    points.push_back({ target.x, exactStart.y });
                }
            } else {
                // پیش‌فرض: رسم یک L-shape تمیز (اول افقی، سپس عمودی)
                points.push_back({ target.x, exactStart.y });
            }
        }

        points.push_back(target);
        SimplifyPoints(points); // قیچی کردن اضافات و تاخوردگی‌ها در صورت وجود
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
            float qx = 0.0f, qy = 0.0f;
            const auto& wire = wires[existingWireIndex];
            float best = HIT_RADIUS;
            for (size_t i = 1; i < wire.points.size(); ++i) {
                const float d = DistancePointToSegment(
                    x, y, wire.points[i - 1].x, wire.points[i - 1].y,
                    wire.points[i].x, wire.points[i].y, &qx, &qy);
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
        AppendOrthogonalPath(activeWire.points, endPoint, gridSpacing);
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
        AppendOrthogonalPath(activeWire.points, endPoint, gridSpacing);
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

    void UpdateEndpointsForComponent(
        int componentId,
        const std::vector<std::unique_ptr<Component>>& components,
        int gridSpacing = 20) {
        const Component* component = FindComponentById(components, componentId);
        if (!component) return;

        for (auto& wire : wires) {
            bool changed = false;
            if (wire.start.componentId == componentId &&
                wire.start.pinIndex >= 0 &&
                wire.start.pinIndex < static_cast<int>(component->pins.size())) {
                const auto pos = component->GetPinWorldPos(component->pins[wire.start.pinIndex]);
                const WirePoint pinPoint = { pos.first, pos.second };
                if (wire.points.empty()) {
                    wire.points.push_back(pinPoint);
                    changed = true;
                } else {
                    wire.points.front() = pinPoint;
                    changed = true;
                    if (wire.points.size() >= 2) {
                        WirePoint next = wire.points[1];
                        wire.points[1] = next;
                        if (std::fabs(pinPoint.x - next.x) > 0.001f && std::fabs(pinPoint.y - next.y) > 0.001f) {
                            wire.points.insert(wire.points.begin() + 1, {next.x, pinPoint.y});
                        }
                    }
                }
            }

            if (wire.end.componentId == componentId &&
                wire.end.pinIndex >= 0 &&
                wire.end.pinIndex < static_cast<int>(component->pins.size())) {
                const auto pos = component->GetPinWorldPos(component->pins[wire.end.pinIndex]);
                const WirePoint pinPoint = { pos.first, pos.second };
                if (wire.points.empty()) {
                    wire.points.push_back(pinPoint);
                    changed = true;
                } else {
                    wire.points.back() = pinPoint;
                    changed = true;
                    if (wire.points.size() >= 2) {
                        WirePoint prev = wire.points[wire.points.size() - 2];
                        if (std::fabs(prev.x - pinPoint.x) > 0.001f && std::fabs(prev.y - pinPoint.y) > 0.001f) {
                            wire.points.insert(wire.points.end() - 1, {pinPoint.x, prev.y});
                        }
                    }
                }
            }

            if (changed) {
                SimplifyPoints(wire.points); // پاکسازی خطوط و تاخوردگی‌ها
            }
        }
        RebuildJunctions(components);
    }

    static void ReflowEndpoint(std::vector<WirePoint>& points, bool isFront, WirePoint newPoint) {
        if (points.empty()) {
            points.push_back(newPoint);
            return;
        }
        if (points.size() == 1) {
            // اگر سیم به یک نقطه فشرده شده بود، دوباره بازش می‌کنیم
            if (isFront) points.insert(points.begin(), newPoint);
            else points.push_back(newPoint);
            SimplifyPoints(points);
            return;
        }
        if (isFront) {
            const WirePoint anchor = points[1];
            points.front() = newPoint;
            if (std::fabs(newPoint.x - anchor.x) > 0.001f && std::fabs(newPoint.y - anchor.y) > 0.001f) {
                points.insert(points.begin() + 1, { anchor.x, newPoint.y });
            }
        } else {
            const WirePoint anchor = points[points.size() - 2];
            points.back() = newPoint;
            if (std::fabs(newPoint.x - anchor.x) > 0.001f && std::fabs(newPoint.y - anchor.y) > 0.001f) {
                points.insert(points.end() - 1, { newPoint.x, anchor.y });
            }
        }
        SimplifyPoints(points); // پاکسازی خطوط و تاخوردگی‌ها
    }

    void MoveComponentWires(
        int componentId,
        const std::vector<std::unique_ptr<Component>>& components,
        int gridSpacing = 20) {
        const Component* component = FindComponentById(components, componentId);
        if (!component) return;

        struct PendingMove {
            WirePoint oldPoint;
            WirePoint newPoint;
        };
        std::vector<PendingMove> queue;
        std::unordered_set<Wire*> settled;

        auto propagateAlongSegment = [&](WirePoint segA, WirePoint segB, WirePoint delta, Wire* movedWire) {
            for (auto& other : wires) {
                if (&other == movedWire || other.points.empty() || settled.count(&other)) continue;
                if (!other.start.isConnected()) {
                    float qx = 0.0f, qy = 0.0f;
                    const float d = DistancePointToSegment(
                        other.points.front().x, other.points.front().y,
                        segA.x, segA.y, segB.x, segB.y, &qx, &qy);
                    if (d <= JUNCTION_RADIUS + 0.5f) {
                        const WirePoint oldP = other.points.front();
                        const WirePoint newP = { oldP.x + delta.x, oldP.y + delta.y };
                        ReflowEndpoint(other.points, true, newP);
                        settled.insert(&other);
                        queue.push_back({ oldP, newP });
                        continue;
                    }
                }
                if (!other.end.isConnected()) {
                    float qx = 0.0f, qy = 0.0f;
                    const float d = DistancePointToSegment(
                        other.points.back().x, other.points.back().y,
                        segA.x, segA.y, segB.x, segB.y, &qx, &qy);
                    if (d <= JUNCTION_RADIUS + 0.5f) {
                        const WirePoint oldP = other.points.back();
                        const WirePoint newP = { oldP.x + delta.x, oldP.y + delta.y };
                        ReflowEndpoint(other.points, false, newP);
                        settled.insert(&other);
                        queue.push_back({ oldP, newP });
                    }
                }
            }
        };

        for (auto& wire : wires) {
            if (wire.points.empty()) continue;
            if (wire.start.componentId == componentId &&
                wire.start.pinIndex >= 0 &&
                wire.start.pinIndex < static_cast<int>(component->pins.size())) {
                const WirePoint oldP = wire.points.front();
                const WirePoint oldAnchor = wire.points.size() >= 2 ? wire.points[1] : oldP;
                const auto pos = component->GetPinWorldPos(component->pins[wire.start.pinIndex]);
                const WirePoint newP = { pos.first, pos.second };
                if (std::fabs(oldP.x - newP.x) > 0.001f || std::fabs(oldP.y - newP.y) > 0.001f) {
                    const WirePoint delta = { newP.x - oldP.x, newP.y - oldP.y };
                    ReflowEndpoint(wire.points, true, newP);
                    queue.push_back({ oldP, newP });
                    propagateAlongSegment(oldP, oldAnchor, delta, &wire);
                }
                settled.insert(&wire);
            }
            if (wire.end.componentId == componentId &&
                wire.end.pinIndex >= 0 &&
                wire.end.pinIndex < static_cast<int>(component->pins.size())) {
                const WirePoint oldP = wire.points.back();
                const WirePoint oldAnchor = wire.points.size() >= 2 ? wire.points[wire.points.size() - 2] : oldP;
                const auto pos = component->GetPinWorldPos(component->pins[wire.end.pinIndex]);
                const WirePoint newP = { pos.first, pos.second };
                if (std::fabs(oldP.x - newP.x) > 0.001f || std::fabs(oldP.y - newP.y) > 0.001f) {
                    const WirePoint delta = { newP.x - oldP.x, newP.y - oldP.y };
                    ReflowEndpoint(wire.points, false, newP);
                    queue.push_back({ oldP, newP });
                    propagateAlongSegment(oldP, oldAnchor, delta, &wire);
                }
                settled.insert(&wire);
            }
        }

        const float eps2 = 1.0f;
        size_t qi = 0;
        while (qi < queue.size()) {
            const WirePoint oldAnchor = queue[qi].oldPoint;
            const WirePoint newAnchor = queue[qi].newPoint;
            ++qi;
            for (auto& wire : wires) {
                if (wire.points.empty() || settled.count(&wire)) continue;
                if (!wire.start.isConnected() &&
                    DistanceSquared(wire.points.front().x, wire.points.front().y, oldAnchor.x, oldAnchor.y) <= eps2) {
                    ReflowEndpoint(wire.points, true, newAnchor);
                    settled.insert(&wire);
                    queue.push_back({ oldAnchor, newAnchor });
                    continue;
                }
                if (!wire.end.isConnected() &&
                    DistanceSquared(wire.points.back().x, wire.points.back().y, oldAnchor.x, oldAnchor.y) <= eps2) {
                    ReflowEndpoint(wire.points, false, newAnchor);
                    settled.insert(&wire);
                    queue.push_back({ oldAnchor, newAnchor });
                    continue;
                }
            }
        }
        RebuildJunctions(components);
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

    std::vector<int> GetConnectedWireIdsAt(float x, float y) const {
        std::vector<int> result;
        auto appendUnique = [&result](int id) {
            if (std::find(result.begin(), result.end(), id) == result.end()) {
                result.push_back(id);
            }
        };
        for (const auto& wire : wires) {
            if (wire.points.size() < 2) continue;
            bool connected = false;
            for (size_t i = 1; i < wire.points.size(); ++i) {
                if (DistancePointToSegment(
                        x, y,
                        wire.points[i - 1].x, wire.points[i - 1].y,
                        wire.points[i].x, wire.points[i].y) <= JUNCTION_RADIUS + 0.5f) {
                    connected = true;
                    break;
                }
            }
            if (connected) appendUnique(wire.id);
        }
        return result;
    }

    static bool SamePin(const ElectricalPinRef& a, const ElectricalPinRef& b) {
        return a.componentId == b.componentId && a.pinIndex == b.pinIndex;
    }

    std::vector<ElectricalNet> BuildElectricalNets(
        const std::vector<std::unique_ptr<Component>>& components) const {
        struct PinKey {
            int componentId;
            int pinIndex;
            bool operator==(const PinKey& o) const {
                return componentId == o.componentId && pinIndex == o.pinIndex;
            }
        };
        struct PinKeyHash {
            size_t operator()(const PinKey& k) const {
                return (static_cast<size_t>(static_cast<unsigned int>(k.componentId)) << 32) ^
                       static_cast<size_t>(static_cast<unsigned int>(k.pinIndex));
            }
        };
        std::vector<ElectricalPinRef> allPins;
        for (const auto& c : components) {
            if (!c) continue;
            for (int i = 0; i < static_cast<int>(c->pins.size()); ++i) {
                allPins.push_back({c->id, i});
            }
        }
        std::unordered_map<PinKey, int, PinKeyHash> pinNode;
        for (int i = 0; i < static_cast<int>(allPins.size()); ++i) {
            pinNode[{allPins[i].componentId, allPins[i].pinIndex}] = i;
        }
        std::vector<std::vector<int>> graph(allPins.size());
        auto link = [&graph](int a, int b) {
            if (a < 0 || b < 0 || a >= static_cast<int>(graph.size()) || b >= static_cast<int>(graph.size()) || a == b) return;
            graph[a].push_back(b);
            graph[b].push_back(a);
        };
        auto endpointNode = [&pinNode](const WireEndpoint& ep) -> int {
            if (!ep.isConnected()) return -1;
            auto it = pinNode.find({ep.componentId, ep.pinIndex});
            return it == pinNode.end() ? -1 : it->second;
        };
        for (const auto& wire : wires) {
            const int a = endpointNode(wire.start);
            const int b = endpointNode(wire.end);
            if (a >= 0 && b >= 0) link(a, b);
        }
        for (const auto& junction : junctions) {
            std::vector<int> junctionPins;
            for (const auto& wire : wires) {
                bool touches = false;
                for (size_t i = 1; i < wire.points.size(); ++i) {
                    if (DistancePointToSegment(
                            junction.x, junction.y,
                            wire.points[i - 1].x, wire.points[i - 1].y,
                            wire.points[i].x, wire.points[i].y) <= JUNCTION_RADIUS + 0.5f) {
                        touches = true;
                        break;
                    }
                }
                if (!touches) continue;
                const int a = endpointNode(wire.start);
                const int b = endpointNode(wire.end);
                if (a >= 0) junctionPins.push_back(a);
                if (b >= 0) junctionPins.push_back(b);
            }
            std::sort(junctionPins.begin(), junctionPins.end());
            junctionPins.erase(std::unique(junctionPins.begin(), junctionPins.end()), junctionPins.end());
            for (size_t i = 1; i < junctionPins.size(); ++i) {
                link(junctionPins[0], junctionPins[i]);
            }
        }
        std::vector<ElectricalNet> nets;
        std::vector<char> visited(allPins.size(), 0);
        int nextNetId = 1;
        for (int root = 0; root < static_cast<int>(allPins.size()); ++root) {
            if (visited[root]) continue;
            ElectricalNet net;
            net.id = nextNetId++;
            std::vector<int> stack{root};
            visited[root] = 1;
            while (!stack.empty()) {
                int n = stack.back();
                stack.pop_back();
                net.pins.push_back(allPins[n]);
                for (int next : graph[n]) {
                    if (!visited[next]) {
                        visited[next] = 1;
                        stack.push_back(next);
                    }
                }
            }
            nets.push_back(std::move(net));
        }
        return nets;
    }

    void NotifyElectricalTopologyChanged(
        const std::vector<std::unique_ptr<Component>>& components) {
        if (onElectricalTopologyChanged) {
            onElectricalTopologyChanged(BuildElectricalNets(components));
        }
    }

    // ====================================================================
    // انتشار ولتاژ روی هر نتِ الکتریکی (بخش ۷.۶ - "تغییر وضعیت پایه‌ها
    // باید مستقیماً روی قطعات جانبیِ متصل به آن‌ها اثر بگذارد").
    //
    // برای هر نت (مجموعه‌ی پین‌های متصل به هم از طریق سیم/جانکشن)، پینی
    // که در همان لحظه isOutput=true دارد به‌عنوان «راننده»ی نت در نظر
    // گرفته می‌شود و ولتاژش روی همه‌ی پین‌های غیرِخروجیِ همان نت کپی
    // می‌شود. این تابع باید هر فریم، پیش از Update() قطعات، فراخوانی شود
    // تا نتیجه‌ی سیکلِ قبلیِ میکروکنترلر/قطعات به قطعات جانبی برسد.
    // ====================================================================
    void PropagateVoltages(std::vector<std::unique_ptr<Component>>& components) const {
        const std::vector<ElectricalNet> nets = BuildElectricalNets(components);

        auto findComponent = [&components](int id) -> Component* {
            for (auto& c : components) if (c && c->id == id) return c.get();
            return nullptr;
        };

        for (const auto& net : nets) {
            bool driven = false;
            float driverVoltage = 0.0f;

            for (const auto& ref : net.pins) {
                Component* comp = findComponent(ref.componentId);
                if (!comp || ref.pinIndex < 0 || ref.pinIndex >= static_cast<int>(comp->pins.size())) continue;
                const Pin& p = comp->pins[ref.pinIndex];
                if (p.isOutput) { driven = true; driverVoltage = p.voltage; }
            }
            if (!driven) continue;

            for (const auto& ref : net.pins) {
                Component* comp = findComponent(ref.componentId);
                if (!comp || ref.pinIndex < 0 || ref.pinIndex >= static_cast<int>(comp->pins.size())) continue;
                Pin& p = comp->pins[ref.pinIndex];
                if (!p.isOutput) p.voltage = driverVoltage;
            }
        }
    }

    void RebuildJunctions(const std::vector<std::unique_ptr<Component>>& components) {
        std::vector<WirePoint> oldCoordinates;
        oldCoordinates.reserve(junctions.size());
        for (const auto& j : junctions) oldCoordinates.push_back({j.x, j.y});
        junctions.clear();
        for (const auto& point : oldCoordinates) {
            const std::vector<int> ids = GetConnectedWireIdsAt(point.x, point.y);
            if (ids.size() >= 2) {
                JunctionDot fresh;
                fresh.x = point.x;
                fresh.y = point.y;
                fresh.connectedWireIds = ids;
                junctions.push_back(std::move(fresh));
            }
        }
        for (size_t i = 0; i < wires.size(); ++i) {
            for (size_t j = i + 1; j < wires.size(); ++j) {
                const auto& a = wires[i];
                const auto& b = wires[j];
                if (a.points.empty() || b.points.empty()) continue;
                const WirePoint aPts[] = {a.points.front(), a.points.back()};
                const WirePoint bPts[] = {b.points.front(), b.points.back()};
                for (const auto& pa : aPts) {
                    for (const auto& pb : bPts) {
                        if (DistanceSquared(pa.x, pa.y, pb.x, pb.y) > 0.01f) continue;
                        const bool aPin =
                            ((pa.x == a.points.front().x && pa.y == a.points.front().y) && a.start.isConnected()) ||
                            ((pa.x == a.points.back().x  && pa.y == a.points.back().y ) && a.end.isConnected());
                        const bool bPin =
                            ((pb.x == b.points.front().x && pb.y == b.points.front().y) && b.start.isConnected()) ||
                            ((pb.x == b.points.back().x  && pb.y == b.points.back().y ) && b.end.isConnected());
                        if (!(aPin && bPin)) AddJunction(pa.x, pa.y);
                    }
                }
            }
        }
        for (auto it = junctions.begin(); it != junctions.end();) {
            it->connectedWireIds = GetConnectedWireIdsAt(it->x, it->y);
            if (it->connectedWireIds.size() < 2) {
                it = junctions.erase(it);
            } else {
                ++it;
            }
        }
        NotifyElectricalTopologyChanged(components);
    }

    void RemoveWireByIndex(
        int wireIndex,
        const std::vector<std::unique_ptr<Component>>& components) {
        if (wireIndex < 0 || wireIndex >= static_cast<int>(wires.size())) return;
        wires.erase(wires.begin() + wireIndex);
        RebuildJunctions(components);
    }

    void DeleteSelected(const std::vector<std::unique_ptr<Component>>& components) {
        bool removedAny = false;
        wires.erase(
            std::remove_if(wires.begin(), wires.end(),
                [&removedAny](const Wire& w) {
                    if (w.isSelected) {
                        removedAny = true;
                        return true;
                    }
                    return false;
                }),
            wires.end()
        );
        if (removedAny) RebuildJunctions(components);
    }

    void DeleteSelected() {
        wires.erase(
            std::remove_if(wires.begin(), wires.end(),
                [](const Wire& w) { return w.isSelected; }),
            wires.end()
        );
        std::vector<JunctionDot> rebuilt;
        for (const auto& old : junctions) {
            const auto ids = GetConnectedWireIdsAt(old.x, old.y);
            if (ids.size() >= 2) {
                JunctionDot fresh{old.x, old.y, ids};
                rebuilt.push_back(std::move(fresh));
            }
        }
        junctions = std::move(rebuilt);
    }

    void DeleteWiresConnectedToComponent(
        int componentId,
        const std::vector<std::unique_ptr<Component>>& components) {
        bool removedAny = false;
        wires.erase(
            std::remove_if(wires.begin(), wires.end(),
                [componentId, &removedAny](const Wire& w) {
                    if (w.start.componentId == componentId ||
                        w.end.componentId == componentId) {
                        removedAny = true;
                        return true;
                    }
                    return false;
                }),
            wires.end()
        );
        if (removedAny) RebuildJunctions(components);
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
        for (auto it = junctions.begin(); it != junctions.end();) {
            it->connectedWireIds = GetConnectedWireIdsAt(it->x, it->y);
            if (it->connectedWireIds.size() < 2) it = junctions.erase(it);
            else ++it;
        }
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
                SDL_RenderLine(renderer, wire.points[i - 1].x, wire.points[i - 1].y, wire.points[i].x, wire.points[i].y);
            }
        }
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
