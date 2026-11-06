# ArmorHUD Integration Guide

## Что было сделано

На основе reversal data из Minecraft 1.21.44 была переделана система работы с прочностью брони в ArmorHUD модуле.

## Основные изменения

### 1. ItemStack.hpp - Новые методы для работы с durability

```cpp
// Получить текущий урон (из mAuxValue @ offset 0x20)
int getDamage() const;

// Получить макс прочность (из Item::mMaxDamage @ offset 0x268)
int getMaxDamage() const;

// Получить оставшуюся прочность
int getRemainingDurability() const;

// Получить процент прочности (0.0 - 1.0)
float getDurabilityPercent() const;

// Проверить есть ли прочность
bool isArmorEnchantable() const;
```

### 2. Item.hpp - Добавлено поле mMaxDamage

```cpp
CLASS_FIELD(int, mMaxDamage, 0x268);  // Из реверса MC 1.21.44
```

### 3. ArmorHUD.cpp - Полный рефактор

- Использует новые методы для получения прочности
- Безопасная работа с указателями
- Правильный расчет процентов
- Цветовая индикация: Зеленый → Желтый → Красный

## Как это работает

```cpp
// 1. Получить контейнер с броней
auto armorContainer = player->getArmorContainer();

// 2. Получить стак брони
ItemStack* stack = armorContainer->getItem(slotIndex); // 0-3

// 3. Получить значения
int damage = stack->getDamage();              // Текущий урон
int maxDamage = stack->getMaxDamage();        // Макс прочность
float durability = stack->getDurabilityPercent(); // 0.0 - 1.0
```

## Reversal Data Used

| Параметр | Значение | Тип |
|----------|----------|-----|
| Item::mMaxDamage | 0x268 | int |
| ItemStackBase::mAuxValue | 0x20 | short |
| query.max_durability | 0x145B73B00 | handler |
| query.remaining_durability | 0x145B73AD0 | handler |

## Структура проекта

```
src/
├── SDK/Minecraft/Inventory/
│   ├── ItemStack.hpp          ← Добавлены методы durability
│   ├── Item.hpp               ← Добавлено mMaxDamage
│   └── ItemStack.cpp
├── Features/Modules/Visual/
│   ├── ArmorHUD.cpp           ← Полностью переделан
│   ├── ArmorHUD.hpp           ← Добавлена документация
│   ├── ARMORHUD_DOCUMENTATION.md
│   └── DURABILITY_EXAMPLES.cpp
└── REVERSAL_REFERENCE.txt     ← Справочник всех данных
```

## Файлы, что были изменены/добавлены

### Изменены:
- ✅ `ItemStack.hpp` - добавлены методы getDamage(), getMaxDamage(), etc.
- ✅ `Item.hpp` - добавлено поле mMaxDamage с offset 0x268
- ✅ `ArmorHUD.cpp` - полный рефактор на использование новых методов
- ✅ `ArmorHUD.hpp` - добавлены комментарии с reversal data

### Добавлены:
- ✅ `ARMORHUD_DOCUMENTATION.md` - полная документация
- ✅ `DURABILITY_EXAMPLES.cpp` - примеры использования в других модулях
- ✅ `REVERSAL_REFERENCE.txt` - справочник всех reversal данных

## Использование в других модулях

### Пример 1: Проверить прочность брони

```cpp
auto armorContainer = player->getArmorContainer();
for (int i = 0; i < 4; i++) {
    ItemStack* stack = armorContainer->getItem(i);
    if (!stack || !stack->mItem) continue;
    
    float durability = stack->getDurabilityPercent();
    if (durability < 0.25f) {
        // Броня критична!
    }
}
```

### Пример 2: Получить худшую броню

```cpp
int worstSlot = 0;
float worstDurability = 1.0f;

for (int i = 0; i < 4; i++) {
    ItemStack* stack = armorContainer->getItem(i);
    if (!stack || !stack->mItem) continue;
    
    float dur = stack->getDurabilityPercent();
    if (dur < worstDurability) {
        worstDurability = dur;
        worstSlot = i;
    }
}
```

### Пример 3: Оповещение о низкой прочности

```cpp
const char* names[] = {"Helmet", "Chest", "Legs", "Boots"};
for (int i = 0; i < 4; i++) {
    ItemStack* stack = armorContainer->getItem(i);
    if (!stack || !stack->mItem) continue;
    
    if (stack->getDurabilityPercent() < 0.1f) {
        chatMessage("⚠ " + std::string(names[i]) + " critical!");
    }
}
```

## Тестирование

1. **Визуализация:**
   - Убедиться что показываются буквы (H, C, L, B)
   - Проверить цветовое градиент: зеленый → желтый → красный
   - Проверить процент текст

2. **Функциональность:**
   - Надеть разную броню с разной прочностью
   - Взять урон и проверить обновление
   - Проверить горизонтальный/вертикальный режим

3. **Производительность:**
   - Нет утечек памяти
   - Правильная работа с пустыми слотами

## Возможные улучшения

- [ ] Использовать Molang queries для альтернативного способа
- [ ] Добавить звуковой эффект при критической прочности
- [ ] Интеграция с AutoArmor модулем
- [ ] Поддержка инструментов (не только броня)
- [ ] Логирование в файл при критической прочности
- [ ] Уведомления через notification system

## Быстрая справка

| Метод | Описание | Возвращаемый тип |
|-------|---------|------------------|
| getDamage() | Текущий урон | int |
| getMaxDamage() | Максимальная прочность | int |
| getRemainingDurability() | Оставшаяся прочность | int |
| getDurabilityPercent() | Процент (0.0-1.0) | float |
| isArmorEnchantable() | Есть ли прочность | bool |

## Reversal Data Reference

```
OFFSETS:
Item::mMaxDamage @ 0x268
ItemStackBase::mAuxValue @ 0x20

FORMULAS:
damage = mAuxValue
max_damage = Item::mMaxDamage
remaining = max_damage - damage
percent = remaining / max_damage

HANDLERS:
query.max_durability: 0x145B73B00
query.remaining_durability: 0x145B73AD0

FUNCTIONS:
sub_143599870: Item serialization
sub_14344B770: DurabilityItemComponent type ID
sub_1435B02A0: Durability component JSON validator
```

## Контакты и поддержка

При возникновении проблем:
1. Проверить что используются новые методы
2. Убедиться что мItem не null
3. Проверить reversal data в файлах комментариев
4. Смотреть примеры в DURABILITY_EXAMPLES.cpp

## История версий

### v1.0 (2026-02-06)
- ✅ Реализация методов getDurability на основе reversal
- ✅ Добавлено поле mMaxDamage в Item
- ✅ Полный рефактор ArmorHUD
- ✅ Создана документация и примеры
- ✅ Добавлен reversal data reference

---

**Minecraft Version:** 1.21.44  
**Last Updated:** 2026-02-06  
**Status:** ✅ Ready for production
