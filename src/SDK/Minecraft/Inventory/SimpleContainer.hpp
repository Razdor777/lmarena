#pragma once
//
//

class Container {
public:
    virtual ~Container() = 0;
    class ItemStack* getItem(int slot);
    void setItem(int slot, class ItemStack* item);
};

class SimpleContainer : public Container {};