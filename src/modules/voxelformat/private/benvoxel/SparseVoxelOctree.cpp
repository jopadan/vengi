// MIT License
//
// Copyright (c) 2024 Ben McLean
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "SparseVoxelOctree.h"
#include "Leaf.h"
#include "io/Stream.h"
#include "voxelformat/private/benvoxel/Node.h"

namespace BenVoxel {

Voxel::Voxel(uint16_t x, uint16_t y, uint16_t z, uint8_t _index) : position(x, y, z), index(_index) {
}

SparseVoxelOctree::SparseVoxelOctree(io::SeekableReadStream &in, uint16_t sizeX, uint16_t sizeY, uint16_t sizeZ)
	: _root(nullptr, in), _sizeX(sizeX), _sizeY(sizeY), _sizeZ(sizeZ) {
}

void SparseVoxelOctree::write(io::SeekableWriteStream &out, bool includeSizes) const {
	if (includeSizes) {
		uint16_t dimensions[3] = {_sizeX, _sizeY, _sizeZ};
		out.write(reinterpret_cast<const char *>(dimensions), sizeof(dimensions));
	}
	_root.write(out);
}

uint8_t SparseVoxelOctree::operator[](Position &position) const {
	return get(position.x, position.y, position.z);
}

static int toIndex(int level, uint16_t x, uint16_t y, uint16_t z) {
	return (z >> level & 1) << 2 | (y >> level & 1) << 1 | (x >> level & 1);
}

uint8_t SparseVoxelOctree::get(uint16_t x, uint16_t y, uint16_t z) const {
	const Branch *branch = &_root;
	for (uint8_t level = 15; level > 1; level--) {
		const int idx = toIndex(level, x, y, z);
		const Node *node = (*branch)[idx];
		if (!isBranch(node)) {
			return 0;
		}
		branch = (const Branch *)node;
	}
	const Node *leaf = (*branch)[toIndex(1, x, y, z)];
	if (isLeaf(leaf)) {
		return (*(const Leaf *)leaf)[toIndex(0, x, y, z)];
	}
	return 0;
}

core::DynamicArray<Voxel> SparseVoxelOctree::voxels() const {
	core::DynamicArray<Voxel> list = {};
	core::DynamicStack<const Branch *> stack = {};
	fillStack(stack, &_root);
	while (!stack.empty()) {
		const Branch *branch = stack.pop();
		if (stack.size() == 14) {
			for (uint8_t octant = 0; octant < 8; octant++) {
				const Node *node = (*branch)[octant];
				if (isLeaf(node)) {
					const Leaf *leaf = (const Leaf *)node;
					const Position &position = leaf->position();
					for (uint8_t childOctant = 0; childOctant < 8; childOctant++) {
						const uint8_t index = (*leaf)[childOctant];
						if (index) {
							list.push_front(Voxel(position.x + (childOctant & 1), position.y + ((childOctant >> 1) & 1),
												  position.z + ((childOctant >> 2) & 1), index));
						}
					}
				}
			}
		}
		const Branch *parent = branch->getParent();
		if (parent) {
			const Node *next = parent->nextValidChild(branch->getOctant());
			if (isBranch(next)) {
				fillStack(stack, (const Branch *)next);
			}
		}
	}
	return list;
}

void SparseVoxelOctree::fillStack(core::DynamicStack<const Branch *> &stack, const Branch *branch) {
	while (branch) {
		stack.push(branch);
		branch = isBranch(branch->first()) ? (Branch *)branch->first() : nullptr;
	}
}

void SparseVoxelOctree::set(Voxel voxel) {
	return set(voxel.position.x, voxel.position.y, voxel.position.z, voxel.index);
}

void SparseVoxelOctree::set(uint16_t x, uint16_t y, uint16_t z, uint8_t index) {
	Branch *branch = &_root;
	uint8_t octant;
	for (uint8_t level = 15; level > 1; level--) {
		octant = toIndex(level, x, y, z);
		Node *node = (*branch)[octant];
		if (isBranch(node)) {
			branch = (Branch *)node;
		} else {
			if (index == 0) {
				return;
			}
			branch->set(new Branch(branch, octant));
			branch = isBranch((*branch)[octant]) ? (Branch *)(*branch)[octant] : nullptr;
		}
	}
	octant = toIndex(1, x, y, z);
	Node *node = (*branch)[octant];
	Leaf *leaf = nullptr;
	if (isLeaf(node)) {
		if (index == 0) {
			return;
		}
		branch->set(new Leaf(branch, octant));
		leaf = isLeaf((*branch)[octant]) ? (Leaf *)(*branch)[octant] : nullptr;
	} else {
		leaf = isLeaf(node) ? (Leaf *)node : nullptr;
	}
	leaf->set(toIndex(0, x, y, z), index);
}

SparseVoxelOctree::~SparseVoxelOctree() {
	clear();
}

void SparseVoxelOctree::clear() {
	_root = Branch();
	_sizeX = UINT16_MAX;
	_sizeY = UINT16_MAX;
	_sizeZ = UINT16_MAX;
}

} // namespace BenVoxel