//
// Created by yves-dylane on 5/14/26.
//
#include "UndoSystem.h"
#include <iostream>

void UndoSystem::push(Command cmd) {
	redoStack.clear(); // new action clears redo
	undoStack.push_back(std::move(cmd));
	if ((int)undoStack.size() > MAX_STACK)
		undoStack.erase(undoStack.begin());
}

void UndoSystem::beginStroke(bool isErase) {
	currentStroke = StrokeCommand{};
	currentStroke.isErase = isErase;
	strokeInProgress = true;
}

void UndoSystem::recordTile(int layer, int x, int y, int oldID, int newID) {
	if (!strokeInProgress) return;
	// Avoid duplicate recordings for same cell in one stroke
	for (auto& t : currentStroke.tiles)
		if (t.layer == layer && t.x == x && t.y == y) return;
	currentStroke.tiles.push_back({layer, x, y, oldID, newID});
}

void UndoSystem::endStroke() {
	if (!strokeInProgress) return;
	strokeInProgress = false;
	if (!currentStroke.tiles.empty())
		push(currentStroke);
}

void UndoSystem::recordPlaceObject(int index, const ObjectInstance& obj) {
	push(PlaceObjectCommand{index, obj});
}

void UndoSystem::recordRemoveObject(int index, const ObjectInstance& obj) {
	push(RemoveObjectCommand{index, obj});
}

Command UndoSystem::undo() {
	Command cmd = undoStack.back();
	undoStack.pop_back();
	redoStack.push_back(cmd);
	return cmd;
}

Command UndoSystem::redo() {
	Command cmd = redoStack.back();
	redoStack.pop_back();
	undoStack.push_back(cmd);
	return cmd;
}