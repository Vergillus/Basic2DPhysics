// Raylib-Various.cpp : This file contains the 'main' function. Program execution begins and ends there.

// WARNING!!!!!!
//**********************************************/
// The code here is not optimized and dirty.
// I wrote this code to quickly test some ideas.
//**********************************************/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <unordered_map>
#include <unordered_set>

#include <random>

#include "raylib.h"
#include "raymath.h"

using std::vector;
using std::string;
using std::unordered_map;
using std::unordered_set;

struct GameObject
{
	Vector2 position;
	Vector2 velocity;
	Vector2 gridIndex;

	float mass;
	float inverseMass;
	float restitution;
	float scale;

	bool bUseGravity;

	GameObject() = default;

	GameObject(const Vector2& position, const Vector2& velocity, const Vector2& gridIndex, const float mass, const float restitution, const float scale, bool bUseGravity = true)
		: position(position), velocity(velocity), gridIndex(gridIndex), mass(mass), restitution(restitution), scale(scale), bUseGravity(bUseGravity)
	{
		inverseMass = mass == 0 ? 0 : 1.0f / mass;
	}
};

struct Vector2Hash
{
	int x, y;

	Vector2Hash() { x = 0; y = 0; }
	Vector2Hash(int inX, int inY) : x(inX), y(inY) {}

	Vector2Hash(const Vector2Hash& o) = default;
	Vector2Hash& operator=(const Vector2Hash& o) = default;

	Vector2Hash& operator=(Vector2Hash&& o) = default;
	Vector2Hash(Vector2Hash&& o) = default;

	bool operator==(const Vector2Hash& o) const = default;	

	std::string ToString() const
	{
		return "x: " + std::to_string(x) + " y:" + std::to_string(y) + "\n";
	}
};

namespace std
{
	template<>
	struct hash<Vector2Hash>
	{
		std::size_t operator()(const Vector2Hash& o) const
		{
			std::size_t h1 = std::hash<int>{}(o.x);
			std::size_t h2 = std::hash<int>{}(o.y);
			return h1 ^ (h2 << 1);
		}
	};
}


void GetGridIndexFromPosition(const Vector2& position, const float cellSize, int& xIndex, int& yIndex);
void GetAllCellNeighbourIndexes(const Vector2& position, const float cellSize, const int horCellAmountvector, const int verCellAmount, vector<Vector2>& cellIndexes);

string Vector2ToString(const Vector2& v);

float RandomFloat(float min, float max);

void ResolveCollision(GameObject* A, GameObject* B);
void PositionalCorrection(GameObject* A, GameObject* B);

int main()
{
	constexpr int screenWidth = 1280;
	constexpr int screenHeight = 720;
	constexpr int halfScreenWidth = screenWidth * 0.5f;
	constexpr int halfScreenHeight = screenHeight * 0.5f;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);

	InitWindow(screenWidth, screenHeight, "TEST");

	SetTargetFPS(60);

	const float cellSize = 80;

	const int horizontalCellAmount = screenWidth / (cellSize);
	const int vecticalCellAmount = screenHeight / (cellSize);

	Rectangle cellRect = { 0,0,cellSize,cellSize };

	Vector2 mousePos;
	Vector2 mouseGridPos;

	vector<Vector2> gridPositions;

	unordered_map<Vector2Hash, unordered_set<GameObject*>> gridCells;

	for (int y = 0; y < vecticalCellAmount; y++)
	{
		for (int x = 0; x < horizontalCellAmount; x++)
		{
			gridPositions.push_back({ x * cellSize, y * cellSize });

			unordered_set<GameObject*> cellData;

			gridCells.insert({ Vector2Hash{ x ,y }, cellData });
		}
	}

	vector<Vector2> neighbourIndexes;

	GameObject ceMouse;
	ceMouse.gridIndex = { -1,-1 };

	vector<GameObject*> cellElements;
	

	while (!WindowShouldClose())
	{
		neighbourIndexes.clear();

		mousePos = GetMousePosition();

		mouseGridPos.x = std::floor(mousePos.x / cellSize);
		mouseGridPos.y = std::floor(mousePos.y / cellSize);		

		for (auto& cellElement : cellElements)
		{
			if (cellElement->gridIndex.x >= 0 && cellElement->gridIndex.x < horizontalCellAmount &&
				cellElement->gridIndex.y >= 0 && cellElement->gridIndex.y < vecticalCellAmount)
			{
				GetAllCellNeighbourIndexes(cellElement->position, cellSize, horizontalCellAmount, vecticalCellAmount, neighbourIndexes);			

				Vector2 currIndex{ std::floor(cellElement->position.x / cellSize), std::floor(cellElement->position.y / cellSize) };

				//std::cout << Vector2ToString(cellElement[0]->gridIndex) << Vector2ToString(currIndex);

				if (cellElement->gridIndex.x != currIndex.x ||
					cellElement->gridIndex.y != currIndex.y)
				{

					Vector2Hash insertIndex(currIndex.x, currIndex.y);
					Vector2Hash removeIndex(cellElement->gridIndex.x, cellElement->gridIndex.y);


					if (gridCells[removeIndex].find(cellElement) != gridCells[removeIndex].end())
					{
						//std::cout << "Remove at " << removeIndex.x << " " << removeIndex.y << "\n";
						gridCells[removeIndex].erase(cellElement);

					}

					//std::cout << "Insert at " << insertIndex.x << " " << insertIndex.y << "\n";
					gridCells[insertIndex].insert(cellElement);

					cellElement->gridIndex = currIndex;
				}
			}
		}
		

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			float rndMassScale = RandomFloat(10.0f, cellSize * 0.5f);
			cellElements.push_back(new GameObject{ mousePos, Vector2{0}, mouseGridPos, rndMassScale, 1.0f, rndMassScale });
		}
		if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
		{
			float rndMassScale = RandomFloat(10.0f, cellSize * 0.5f);
			cellElements.push_back(new GameObject{ mousePos, Vector2{0}, mouseGridPos, rndMassScale, 1.0f, rndMassScale, false });
		}	

		for (auto& element : cellElements)
		{
			if (element->bUseGravity)
			{
				if (element->position.y < screenHeight - element->scale)
				{
					element->velocity.y += GetFrameTime() * 9.81f;					
				}				
			}

			element->position = Vector2Add(element->position, element->velocity);

			if (element->position.y >= screenHeight - element->scale)
			{
				element->position.y = screenHeight - element->scale;
				element->velocity.y *= -0.8f;
			}
			if (element->position.x <= element->scale || element->position.x >= screenWidth - element->scale)
			{
				element->velocity.x *= -1;
			}
		}	

		for (size_t i = 0; i < cellElements.size(); i++)
		{
			for (size_t j = i; j < cellElements.size(); j++)
			{
				if (i == j) continue;

				GameObject* go1 = cellElements[i];
				GameObject* go2 = cellElements[j];
				if (CheckCollisionCircles(go1->position, go1->scale, go2->position, go2->scale))
				{
					//std::cout << "Collision\n";
					ResolveCollision(go1, go2);
					PositionalCorrection(go1, go2);
				}
			}
		}

		BeginDrawing();

		ClearBackground(GRAY);


		// Check neighbors
		for (size_t i = 0; i < gridPositions.size(); i++)
		{
			cellRect.x = gridPositions[i].x;
			cellRect.y = gridPositions[i].y;

			int xIndex;
			int yIndex;

			GetGridIndexFromPosition(gridPositions[i], cellSize, xIndex, yIndex);

			Color cellColor = RED;
			bool bIsContains = std::any_of(neighbourIndexes.begin(), neighbourIndexes.end(), [&xIndex, &yIndex](const Vector2& index)
				{
					return index.x == xIndex && index.y == yIndex;
				}
			);

			if (bIsContains)
			{
				cellColor = GREEN;
			}

			DrawRectangleLinesEx(cellRect, 1, cellColor);
			DrawText((std::to_string(xIndex) + " " + std::to_string(yIndex)).c_str(), cellRect.x + 20, cellRect.y + 20, 20, BLACK);
			DrawFPS(screenWidth - 100, screenHeight - 100);
		}

		for (const auto& element : cellElements)
		{
			Color clr = element->inverseMass == 0 ? BLACK : WHITE;			
			DrawPolyLinesEx(element->position, 16, element->scale, 0, 5, clr);
		}

		EndDrawing();

	}

  CloseWindow();

	for (auto& it : gridCells)
	{
		for (auto& gameobject : it.second)
		{
			delete(gameobject);
		}
	}
}

void GetGridIndexFromPosition(const Vector2& position, const float cellSize, int& xIndex, int& yIndex)
{
	xIndex = (int)position.x / cellSize; //std::floor(position.x / cellSize);
	yIndex = (int)position.y / cellSize; //std::floor(position.y / cellSize);
}


void GetAllCellNeighbourIndexes(const Vector2& position, const float cellSize, const int horCellAmountvector, const int verCellAmount, vector<Vector2>& cellIndexes)
{
	int currentCellIndexX, currentCellIndexY;

	GetGridIndexFromPosition(position, cellSize, currentCellIndexX, currentCellIndexY);

	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			Vector2 index = { currentCellIndexX + x, currentCellIndexY + y };

			if (index.x < 0 || index.y < 0 ||
				index.x >= horCellAmountvector || index.y >= verCellAmount) continue;

			cellIndexes.push_back(index);

		}
	}
}

string Vector2ToString(const Vector2& v)
{
	return "X: " + std::to_string(v.x) + "Y: " + std::to_string(v.y) + "\n";
}

float RandomFloat(float min, float max)
{

	if (min == max) return min;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dist(min, max);

	return dist(gen);
}

void ResolveCollision(GameObject* A, GameObject* B)
{
	// Calculate relative velocity
	Vector2 rV = Vector2Subtract(B->velocity, A->velocity);

	// Calculate collision normal
	Vector2 collNormal = Vector2Normalize((Vector2Subtract(B->position, A->position)));

	// Calculate relative velocity in terms of the normal direction
	float velAlongNormal = Vector2DotProduct((rV), collNormal);

	// Do not resolve if velocities are separating ( objects are moving away from each other)
	if (velAlongNormal > 0) return;

	// Calculate restitution
	float e = std::min(A->restitution, B->restitution);	
	// Calculate impulse scalar
	float j = -(1.0f + e) * velAlongNormal;
	
	float inverseMassSum = A->inverseMass + B->inverseMass;

	j /= inverseMassSum;

	// Apply impulse
	Vector2 impulse = Vector2Scale(collNormal, j);	

	A->velocity = Vector2Subtract(A->velocity, Vector2Scale(impulse, A->inverseMass));
	B->velocity = Vector2Add(B->velocity, Vector2Scale(impulse, B->inverseMass));
	
}

void PositionalCorrection(GameObject* A, GameObject* B)
{
	const float percent = 0.2f;
	const float slop = 0.01f;

	// Vector from A to B
	Vector2 n = Vector2Subtract(B->position, A->position);

	float d = Vector2Length(n);

	float r = A->scale + B->scale;
	//r *= r;

	float penetration = A->scale;
	if (d != 0)
	{
		penetration = r - d;
	}
	
	Vector2 correction = Vector2Scale(n, std::max(penetration - slop, 0.0f) / (A->inverseMass + B->inverseMass) * percent);
	correction.x *= GetFrameTime();
	correction.y *= GetFrameTime();

	if (A->inverseMass != 0)
	{
		A->position = Vector2Subtract(A->position, Vector2Scale(correction, A->inverseMass));
	}

	if (B->inverseMass != 0)
	{
		B->position = Vector2Add(B->position, Vector2Scale(correction, B->inverseMass));
	}
}
