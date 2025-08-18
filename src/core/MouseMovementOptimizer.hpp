#pragma once

#include "Event.hpp"
#include <vector>
#include <memory>

namespace MouseRecorder::Core
{

/**
 * @brief Utility class for optimizing mouse movement sequences
 *
 * This class provides algorithms to reduce redundant mouse movements
 * while preserving the essential path and timing information.
 */
class MouseMovementOptimizer
{
  public:
    /**
     * @brief Optimization strategy for mouse movements
     */
    enum class OptimizationStrategy
    {
        DistanceThreshold, // Remove points within threshold distance
        DouglasPeucker,    // Douglas-Peucker line simplification
        TimeBased,         // Remove points within time threshold
        Combined           // Combination of multiple strategies
    };

    /**
     * @brief Configuration for mouse movement optimization
     */
    struct OptimizationConfig
    {
        bool enabled{true};
        OptimizationStrategy strategy{OptimizationStrategy::Combined};
        int distanceThreshold{5};          // pixels
        int timeThresholdMs{16};           // milliseconds (~60fps)
        double douglasPeuckerEpsilon{2.0}; // tolerance for line simplification
        bool preserveClicks{true};         // always preserve click positions
        bool preserveFirstLast{
            true}; // always preserve first and last positions
    };

    /**
     * @brief Optimize a sequence of events by reducing redundant mouse
     * movements
     * @param events Input vector of events (will be modified in place)
     * @param config Optimization configuration
     * @return Number of events removed during optimization
     */
    static size_t optimizeEvents(std::vector<std::unique_ptr<Event>>& events,
                                 const OptimizationConfig& config);

    /**
     * @brief Extract just mouse move events from event sequence
     * @param events Input events
     * @param mouseMoveIndices Output indices of mouse move events
     * @return Vector of mouse move events with their original indices
     */
    static std::vector<std::pair<size_t, const Event*>> extractMouseMoveEvents(
        const std::vector<std::unique_ptr<Event>>& events);

    /**
     * @brief Apply distance-based threshold optimization
     * @param mouseMoves Vector of mouse move events with indices
     * @param threshold Distance threshold in pixels
     * @param preserveFirstLast Whether to preserve first and last events
     * @return Set of indices to remove
     */
    static std::vector<size_t> applyDistanceThreshold(
        const std::vector<std::pair<size_t, const Event*>>& mouseMoves,
        int threshold,
        bool preserveFirstLast = true);

    /**
     * @brief Apply Douglas-Peucker line simplification algorithm
     * @param mouseMoves Vector of mouse move events with indices
     * @param epsilon Tolerance for line simplification
     * @return Set of indices to keep (not remove)
     */
    static std::vector<size_t> applyDouglasPeucker(
        const std::vector<std::pair<size_t, const Event*>>& mouseMoves,
        double epsilon);

    /**
     * @brief Apply time-based optimization (remove events too close in time)
     * @param mouseMoves Vector of mouse move events with indices
     * @param timeThresholdMs Time threshold in milliseconds
     * @param preserveFirstLast Whether to preserve first and last events
     * @return Set of indices to remove
     */
    static std::vector<size_t> applyTimeThreshold(
        const std::vector<std::pair<size_t, const Event*>>& mouseMoves,
        int timeThresholdMs,
        bool preserveFirstLast = true);

    /**
     * @brief Calculate distance between two points
     * @param p1 First point
     * @param p2 Second point
     * @return Distance in pixels
     */
    static double calculateDistance(const Point& p1, const Point& p2);

    /**
     * @brief Calculate perpendicular distance from point to line segment
     * @param point Point to calculate distance for
     * @param lineStart Start of line segment
     * @param lineEnd End of line segment
     * @return Perpendicular distance
     */
    static double perpendicularDistance(const Point& point,
                                        const Point& lineStart,
                                        const Point& lineEnd);

    /**
     * @brief Remove events at specified indices from vector
     * @param events Vector to remove events from
     * @param indicesToRemove Sorted vector of indices to remove
     */
    static void removeEventsAtIndices(
        std::vector<std::unique_ptr<Event>>& events,
        const std::vector<size_t>& indicesToRemove);

  private:
    /**
     * @brief Recursive Douglas-Peucker implementation
     */
    static void douglasPeuckerRecursive(
        const std::vector<std::pair<size_t, const Event*>>& mouseMoves,
        size_t startIdx,
        size_t endIdx,
        double epsilon,
        std::vector<bool>& keep);
};

} // namespace MouseRecorder::Core
