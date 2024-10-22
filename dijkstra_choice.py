import heapq
from collections import defaultdict

def list_paths_dijkstra(graph, start, target):
    distances = defaultdict(lambda: float('inf'))
    paths = defaultdict(list)
    distances[start] = 0
    paths[start] = [[start]]
    pq = [(0, start)]
    
    while pq:
        curr_dist, node = heapq.heappop(pq)
        
        if curr_dist > distances[node]:
            continue
        
        for neighbor, weight in graph[node]:
            new_dist = curr_dist + weight
            if new_dist < distances[neighbor]:
                distances[neighbor] = new_dist
                paths[neighbor] = [path + [neighbor] for path in paths[node]]
                heapq.heappush(pq, (new_dist, neighbor))
            elif new_dist == distances[neighbor]:
                paths[neighbor].extend([path + [neighbor] for path in paths[node]])
    
    return paths[target]

# Example usage:
graph = {
    'A': [('B', 1), ('C', 1)],
    'B': [('D', 1)],
    'C': [('D', 1)],
    'D': []
}
start = 'A'
target = 'D'
print(list_paths_dijkstra(graph, start, target))  # Output: All shortest paths from 'A' to 'D'

