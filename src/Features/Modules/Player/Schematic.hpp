#pragma once
//
// Schematic - Structure for storing copied blocks
//

#include <vector>
#include <map>
#include <string>
#include <glm/glm.hpp>

struct SchematicBlock {
    glm::ivec3 relativePos;     // Позиция относительно origin
    std::string blockName;       // Название блока
    unsigned int runtimeId;      // Runtime ID для быстрого размещения
    
    SchematicBlock() : relativePos(0), blockName(""), runtimeId(0) {}
    SchematicBlock(glm::ivec3 pos, const std::string& name, unsigned int rtId)
        : relativePos(pos), blockName(name), runtimeId(rtId) {}
};

struct Schematic {
    std::string name;
    glm::ivec3 size;
    glm::ivec3 origin;
    std::vector<SchematicBlock> blocks;
    std::map<std::string, int> blockCounts;
    
    Schematic() : name("unnamed"), size(0), origin(0) {}
    
    void clear() {
        blocks.clear();
        blockCounts.clear();
        size = glm::ivec3(0);
        origin = glm::ivec3(0);
    }
    
    int getTotalBlocks() const {
        int total = 0;
        for (const auto& [name, count] : blockCounts) {
            total += count;
        }
        return total;
    }
    
    bool isEmpty() const {
        return blocks.empty();
    }
};