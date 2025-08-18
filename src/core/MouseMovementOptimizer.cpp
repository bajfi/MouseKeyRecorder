#include "MouseMovementOptimizer.hpp"
#include <algorithm>
#include <math.h>
#include <ranges>
#include <set>
#include <spdlog/spdlog.h>

namespace MouseRecorder::Core
{

size_t MouseMovementOptimizer::optimizeEvents(
    std::vector<std::unique_ptr<Event>>& events,
    const OptimizationConfig& config)
{
    if (!config.enabled || events.empty())
    {
        return 0;
    }

    spdlog::debug("MouseMovementOptimizer: Starting optimization of {} events",
                  events.size());
    size_t originalSize = events.size();

    // Extract mouse move events with their indices
    auto mouseMoves = extractMouseMoveEvents(events);
    if (mouseMoves.size() < 3)
    {
        spdlog::debug(
            "MouseMovementOptimizer: Too few mouse moves to optimize");
        return 0;
    }

    std::vector<size_t> indicesToRemove;

    switch (config.strategy)
    {
    case OptimizationStrategy::DistanceThreshold: {
        auto toRemove = applyDistanceThreshold(
            mouseMoves, config.distanceThreshold, config.preserveFirstLast);
        indicesToRemove.insert(
            indicesToRemove.end(), toRemove.begin(), toRemove.end());
        break;
    }

    case OptimizationStrategy::DouglasPeucker: {
        auto toKeep =
            applyDouglasPeucker(mouseMoves, config.douglasPeuckerEpsilon);
        // Convert "keep" indices to "remove" indices
        std::set<size_t> keepSet(toKeep.begin(), toKeep.end());
        for (const auto& move : mouseMoves)
        {
            if (keepSet.find(move.first) == keepSet.end())
            {
                indicesToRemove.push_back(move.first);
            }
        }
        break;
    }

    case OptimizationStrategy::TimeBased: {
        auto toRemove = applyTimeThreshold(
            mouseMoves, config.timeThresholdMs, config.preserveFirstLast);
        indicesToRemove.insert(
            indicesToRemove.end(), toRemove.begin(), toRemove.end());
        break;
    }

    case OptimizationStrategy::Combined: {
        // Apply multiple strategies in sequence
        // 1. First apply time-based filtering
        auto timeRemove = applyTimeThreshold(
            mouseMoves, config.timeThresholdMs, config.preserveFirstLast);

        // 2. Remove time-filtered events from mouseMoves temporarily
        std::set<size_t> timeRemoveSet(timeRemove.begin(), timeRemove.end());
        std::vector<std::pair<size_t, const Event*>> filteredMoves;
        for (const auto& move : mouseMoves)
        {
            if (timeRemoveSet.find(move.first) == timeRemoveSet.end())
            {
                filteredMoves.push_back(move);
            }
        }

        // 3. Apply Douglas-Peucker on remaining moves
        if (filteredMoves.size() >= 3)
        {
            auto toKeep = applyDouglasPeucker(filteredMoves,
                                              config.douglasPeuckerEpsilon);
            std::set<size_t> keepSet(toKeep.begin(), toKeep.end());

            for (const auto& [index, move] : filteredMoves)
            {
                if (keepSet.find(index) == keepSet.end())
                {
                    indicesToRemove.push_back(index);
                }
            }
        }

        // Add time-based removals
        indicesToRemove.insert(
            indicesToRemove.end(), timeRemove.begin(), timeRemove.end());
        break;
    }
    }

    // If preserveClicks is enabled, don't remove mouse moves that are
    // immediately before or after clicks
    if (config.preserveClicks)
    {
        std::set<size_t> clickAdjacent;
        for (size_t i = 0; i < events.size(); ++i)
        {
            const auto& event = events[i];
            if (event->getType() == EventType::MouseClick ||
                event->getType() == EventType::MouseDoubleClick)
            {
                // Preserve mouse moves before and after clicks
                if (i > 0 && events[i - 1]->getType() == EventType::MouseMove)
                {
                    clickAdjacent.insert(i - 1);
                }
                if (i + 1 < events.size() &&
                    events[i + 1]->getType() == EventType::MouseMove)
                {
                    clickAdjacent.insert(i + 1);
                }
            }
        }

        // Remove click-adjacent indices from removal list
        indicesToRemove.erase(std::remove_if(indicesToRemove.begin(),
                                             indicesToRemove.end(),
                                             [&clickAdjacent](size_t idx)
                                             {
                                                 return clickAdjacent.count(
                                                            idx) > 0;
                                             }),
                              indicesToRemove.end());
    }

    // Sort indices in descending order for safe removal
    std::ranges::stable_sort(indicesToRemove);

    // Remove duplicates
    indicesToRemove.erase(
        std::unique(indicesToRemove.begin(), indicesToRemove.end()),
        indicesToRemove.end());

    // Remove the events
    removeEventsAtIndices(events, indicesToRemove);

    size_t removedCount = originalSize - events.size();
    spdlog::debug("MouseMovementOptimizer: Removed {} events, {} remaining",
                  removedCount,
                  events.size());

    return removedCount;
}

std::vector<std::pair<size_t, const Event*>> MouseMovementOptimizer::
    extractMouseMoveEvents(const std::vector<std::unique_ptr<Event>>& events)
{
    std::vector<std::pair<size_t, const Event*>> mouseMoves;

    for (size_t i = 0; i < events.size(); ++i)
    {
        if (events[i]->getType() == EventType::MouseMove)
        {
            mouseMoves.emplace_back(i, events[i].get());
        }
    }

    return mouseMoves;
}

std::vector<size_t> MouseMovementOptimizer::applyDistanceThreshold(
    const std::vector<std::pair<size_t, const Event*>>& mouseMoves,
    int threshold,
    bool preserveFirstLast)
{
    std::vector<size_t> toRemove;

    if (mouseMoves.size() < 3)
    {
        return toRemove;
    }

    Point lastKeptPosition;
    if (const auto* mouseData = mouseMoves[0].second->getMouseData())
    {
        lastKeptPosition = mouseData->position;
    }

    // Start from index 1, skip first if preserving
    size_t startIdx = preserveFirstLast ? 1 : 0;
    size_t endIdx =
        preserveFirstLast ? mouseMoves.size() - 1 : mouseMoves.size();

    for (size_t i = startIdx; i < endIdx; ++i)
    {
        const auto* mouseData = mouseMoves[i].second->getMouseData();
        if (!mouseData)
        {
            continue;
        }

        double distance =
            calculateDistance(lastKeptPosition, mouseData->position);
        if (distance < threshold)
        {
            toRemove.push_back(mouseMoves[i].first);
        }
        else
        {
            lastKeptPosition = mouseData->position;
        }
    }

    return toRemove;
}

std::vector<size_t> MouseMovementOptimizer::applyDouglasPeucker(
    const std::vector<std::pair<size_t, const Event*>>& mouseMoves,
    double epsilon)
{
    if (mouseMoves.size() < 3)
    {
        std::vector<size_t> result;
        for (const auto& [index, move] : mouseMoves)
        {
            result.push_back(index);
        }
        return result;
    }

    std::vector<bool> keep(mouseMoves.size(), false);

    // Always keep first and last points
    keep[0] = true;
    keep[mouseMoves.size() - 1] = true;

    douglasPeuckerRecursive(
        mouseMoves, 0, mouseMoves.size() - 1, epsilon, keep);

    std::vector<size_t> result;
    for (size_t i = 0; i < keep.size(); ++i)
    {
        if (keep[i])
        {
            result.push_back(mouseMoves[i].first);
        }
    }

    return result;
}

std::vector<size_t> MouseMovementOptimizer::applyTimeThreshold(
    const std::vector<std::pair<size_t, const Event*>>& mouseMoves,
    int timeThresholdMs,
    bool preserveFirstLast)
{
    std::vector<size_t> toRemove;

    if (mouseMoves.size() < 3)
    {
        return toRemove;
    }

    auto lastKeptTime = mouseMoves[0].second->getTimestamp();

    // Start from index 1, skip first if preserving
    size_t startIdx = preserveFirstLast ? 1 : 0;
    size_t endIdx =
        preserveFirstLast ? mouseMoves.size() - 1 : mouseMoves.size();

    for (size_t i = startIdx; i < endIdx; ++i)
    {
        auto currentTime = mouseMoves[i].second->getTimestamp();
        auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(
                            currentTime - lastKeptTime)
                            .count();

        if (timeDiff < timeThresholdMs)
        {
            toRemove.push_back(mouseMoves[i].first);
        }
        else
        {
            lastKeptTime = currentTime;
        }
    }

    return toRemove;
}

double MouseMovementOptimizer::calculateDistance(const Point& p1,
                                                 const Point& p2)
{
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    return std::hypot(dx, dy);
}

double MouseMovementOptimizer::perpendicularDistance(const Point& point,
                                                     const Point& lineStart,
                                                     const Point& lineEnd)
{
    double dx = lineEnd.x - lineStart.x;
    double dy = lineEnd.y - lineStart.y;

    if (dx == 0 && dy == 0)
    {
        // Line start and end are the same point
        return calculateDistance(point, lineStart);
    }

    double t = ((point.x - lineStart.x) * dx + (point.y - lineStart.y) * dy) /
               (dx * dx + dy * dy);

    t = std::clamp(t, 0.0, 1.0); // Clamp t to [0, 1]

    Point projection{static_cast<int>(lineStart.x + t * dx),
                     static_cast<int>(lineStart.y + t * dy)};

    return calculateDistance(point, projection);
}

void MouseMovementOptimizer::removeEventsAtIndices(
    std::vector<std::unique_ptr<Event>>& events,
    const std::vector<size_t>& indicesToRemove)
{
    // Remove in reverse order to maintain index validity
    for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); ++it)
    {
        if (*it < events.size())
        {
            events.erase(events.begin() + *it);
        }
    }
}

void MouseMovementOptimizer::douglasPeuckerRecursive(
    const std::vector<std::pair<size_t, const Event*>>& mouseMoves,
    size_t startIdx,
    size_t endIdx,
    double epsilon,
    std::vector<bool>& keep)
{
    if (endIdx <= startIdx + 1)
    {
        return;
    }

    // Find the point with maximum distance from line segment
    double maxDistance = 0.0;
    size_t maxIndex = startIdx;

    const auto* startMouseData = mouseMoves[startIdx].second->getMouseData();
    const auto* endMouseData = mouseMoves[endIdx].second->getMouseData();

    if (!startMouseData || !endMouseData)
    {
        return;
    }

    for (size_t i = startIdx + 1; i < endIdx; ++i)
    {
        const auto* mouseData = mouseMoves[i].second->getMouseData();
        if (!mouseData)
        {
            continue;
        }

        double distance = perpendicularDistance(mouseData->position,
                                                startMouseData->position,
                                                endMouseData->position);

        if (distance > maxDistance)
        {
            maxDistance = distance;
            maxIndex = i;
        }
    }

    // If max distance is greater than epsilon, keep the point and recurse
    if (maxDistance > epsilon)
    {
        keep[maxIndex] = true;
        douglasPeuckerRecursive(mouseMoves, startIdx, maxIndex, epsilon, keep);
        douglasPeuckerRecursive(mouseMoves, maxIndex, endIdx, epsilon, keep);
    }
}

} // namespace MouseRecorder::Core
