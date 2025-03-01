/*!*****************************************************************************
\file Graph.h
\author Jazz Teoh Yu Jue
\par DP email: j.teoh\@digipen.edu
\date 25-10-2023
This file contains the declaration of graph, made using the adjacent list
method, and dijkstra algorithm to find a path.

To use the AStar pathfinding, user must first create the graph using GraphData.
Then generate ALGraph using the GraphData created, thereafter use the AStar 
pathfinding algorithm in ALGraph.
*******************************************************************************/
#pragma once
#include "pch.h"
#include <glm/glm.hpp>

void TestGraph();


class ALGraph;

class GraphData {
public:
  GraphData() : mData() {}
  GraphData(std::string const& _filePath); // Load in graph from file path
  void SaveGraph(std::string const& _filePath); // Save graph into file path

  void AddDEdge(glm::vec3 src, glm::vec3 dst); // This version will add dst to point if it can't find dst as a point
  void AddDEdgeSafe(glm::vec3 src, glm::vec3 dst); // This version will not add dst to point if it can't find dst as a point
  void AddUEdge(glm::vec3 p0, glm::vec3 p1);
  void AddPoint(glm::vec3 point);
  // Delete a point and all edges connecting to it
  void DeletePoint(glm::vec3 _point);
  void DeleteUEdge(glm::vec3 p0, glm::vec3 p1);
  void DeleteDEdge(glm::vec3 src, glm::vec3 dst);
  bool CheckForEdge(glm::vec3 src, glm::vec3 dst);
  // Get the point's edges. If can't find point, make a new point
  std::vector<glm::vec3>& GetPointEdges(glm::vec3 point);

  // Convert this graph data into a ALGraph
  std::shared_ptr<ALGraph> MakeALGraph(); // Make a non-dynamic container for efficiency


  std::vector<std::pair<glm::vec3, std::vector<glm::vec3>>> mData;

  // The format to decode is "(x,y,z)"
  static glm::vec3 StrToVec3(std::string const& str);
  static std::string Vec3ToStr(glm::vec3 const& v);
};

class ALGraph {
public:
  ALGraph(int _size) { mData.reserve(_size + 2); mData.resize(_size); }
  ALGraph& operator=(ALGraph const& _graph);

  enum NODE_STATE {
    NONE,
    OPEN,
    CLOSE
  };

  struct AdjList;

  struct Edge {
    AdjList* vertex;    // Vertex id in the data 
    float distance;     // Distance from owner of edge to the vertex in the edge
  };

  // This is like a node
  struct AdjList {
    NODE_STATE state = NONE;
    glm::vec3 point;
    std::vector<Edge> edges; // Vertices that this vertex is pointing to
    AdjList* parent;
    float gCost;
    float hCost;
  };

  void Print();
  static void PrintVec(glm::vec3 v) { std::cout << v.x << "|" << v.y << "|" << v.z; }
  std::vector<AdjList> mData;
public:
//------------------------------
// AStar pathfinding portion
//------------------------------
  
  class OpenList {
  public:
    void Insert(AdjList* node);   // O(1)
    AdjList* PopLowestCostNode(); // O(n)
    bool IsEmpty();

    void Print() {
      std::cout << "OpenList: ";
      for (auto& adjList : mData) {
        glm::vec3 point = adjList->point;
        std::cout << "(";
        PrintVec(point);
        std::cout << ", weight: " << adjList->gCost + adjList->hCost << "), ";
      }
      std::cout << '\n';
    }
  private:
    // Sort the container, put largest fCost infront and smallest fCost behind
    std::vector<AdjList*> mData;
    void Sort();
  };

  //struct AStarSetting {
  //  float elevation;    // What is the elevation range limit for start and end node to connect with other nodes. between -elevation and elevation.
  //} mSetting;

  //AStarSetting& GetSetting() { return mSetting; }

  // Put the G cost and H cost in the AdjList (G cost is accumulated from path, H cost is cost to end point)

  // Add starting and ending adjList into mData at the back, so startnode = end-2, endnode = end-1
  // Do line of sight check with all the nodes and generate the edges for start and end node
  // Calculate G and H cost for every nodes
  std::vector<glm::vec3> AStarPath(); // Generates the AStar path for the graph
  static float CalcHCost(glm::vec3 const& p0, glm::vec3 const& p1);    // TODO find out the most suitable heuristic cost function
private:
  void AStarInit(glm::vec3 const& start, glm::vec3 const& end);

  // Connects the start node and end node to graph
  //void ConnectStartAndEnd();
  void AStarExit();
  std::vector<glm::vec3> MasterPath(AdjList* tailNode);
};


