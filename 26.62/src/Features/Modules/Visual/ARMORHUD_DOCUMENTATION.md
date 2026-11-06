# ArmorHUD Module - Reverse Engineering Implementation

## Обновление на основе реверса MC 1.21.44

### Ключевые изменения

#### 1. **Добавлены методы работы с durability в ItemStack** (`ItemStack.hpp`)
   
```cpp
// Получить текущий урон (из mAuxValue offset 0x20)
int getDamage() const { return static_cast<int>(mAuxValue); }

// Получить макс прочность (из Item::mMaxDamage offset 0x268)
int getMaxDamage() const { /* ... */ }

// Получить оставшуюся прочность
int getRemainingDurability() const { /* ... */ }

// Получить процент прочности (0.0 - 1.0)
float getDurabilityPercent() const { /* ... */ }

// Проверить есть ли прочность у предмета
bool isArmorEnchantable() const { /* ... */ }
```

#### 2. **Добавлено поле mMaxDamage в Item** (`Item.hpp`)

```cpp
CLASS_FIELD(int, mMaxDamage, 0x268);  // Offset из реверса
```

### Reversal Data Used (Minecraft 1.21.44)

```
Item::mMaxDamage offset: 0x268 (616 decimal)
ItemStackBase::mAuxValue offset: 0x20 (32 decimal) - contains damage
```

### Как это работает

1. **Получение урона предмета:**
   - `stack->getDamage()` читает `mAuxValue` - текущий урон
   
2. **Получение макс прочности:**
   - `item->mMaxDamage` берется из offset 0x268 структуры Item

3. **Расчет процента прочности:**
   ```cpp
   durability_percent = (max_damage - current_damage) / max_damage
   ```

4. **Визуализация:**
   - Зеленая полоска (100%) → Желтая (50%) → Красная (0%)
   - Отображение букв (H, C, L, B) для слотов
   - Процент прочности в тексте

### Файлы, которые были изменены

1. **ArmorHUD.cpp** - Полный рефактор на использование новых методов
2. **ArmorHUD.hpp** - Добавлены комментарии с reversal data
3. **ItemStack.hpp** - Добавлены методы для получения durability
4. **Item.hpp** - Добавлено поле mMaxDamage

### Использованные функции из реверса

```
query.max_durability - handler: 0x145B73B00 (returns float)
query.remaining_durability - handler: 0x145B73AD0 (returns float)
sub_143599870 - Item сериализация (содержит offset 616 для damage)
sub_14344B770 - DurabilityItemComponent type ID
```

### Тестирование

1. Надеть броню с разной прочностью
2. Убедиться, что отображается процент
3. Проверить цветовую индикацию прочности
4. Проверить горизонтальную и вертикальную раскладку

### Возможные улучшения в будущем

- Использовать query.remaining_durability для альтернативного метода
- Добавить звуковой эффект при низкой прочности
- Интеграция с другими модулями (уведомления, логирование)
- Поддержка прочности средств (топор, кирка и т.д.)
