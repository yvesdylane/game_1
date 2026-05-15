//
// Created by yves-dylane on 5/14/26.
//

#ifndef GAME1_UNDOSYSTEM_H
#define GAME1_UNDOSYSTEM_H
#pragma once
#include <vector>
#include <variant>
#include "../World/ObjectInstance.h"

// ── Command types ─────────────────────────────────────────────────────────────

struct PaintCommand {
	int layer, x, y;
	int oldTileID;
	int newTileID;
};

struct EraseCommand {
	int layer, x, y;
	int oldTileID;
};

struct PlaceObjectCommand {
	int             objectIndex;
	ObjectInstance  object;
};

struct RemoveObjectCommand {
	int             objectIndex;
	ObjectInstance  object;
};

// A stroke = multiple tile changes from one mouse press+release
struct StrokeCommand {
	std::vector<PaintCommand> tiles;   // all tiles painted this stroke
	bool isErase = false;
};

using Command = std::variant<StrokeCommand, PlaceObjectCommand, RemoveObjectCommand>;

// ── UndoSystem ────────────────────────────────────────────────────────────────

class UndoSystem {
public:
	static constexpr int MAX_STACK = 50;

	void beginStroke(bool isErase = false);
	void recordTile(int layer, int x, int y, int oldID, int newID);
	void endStroke();   // commits current stroke to undo stack

	void recordPlaceObject(int index, const ObjectInstance& obj);
	void recordRemoveObject(int index, const ObjectInstance& obj);

	bool canUndo() const { return !undoStack.empty(); }
	bool canRedo() const { return !redoStack.empty(); }

	// Returns the command to apply in reverse
	Command undo();
	Command redo();

	void clear() { undoStack.clear(); redoStack.clear(); }

private:
	std::vector<Command> undoStack;
	std::vector<Command> redoStack;
	StrokeCommand        currentStroke;
	bool                 strokeInProgress = false;

	void push(Command cmd);
};

#endif //GAME1_UNDOSYSTEM_H