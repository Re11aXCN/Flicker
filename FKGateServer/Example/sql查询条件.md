在 MySQL 8.0 中，`WHERE` 子句支持多种条件写法，以下是最全面的分类和示例：

---

### 1. **基础条件**
#### 等值条件
```sql
SELECT * FROM users WHERE id = 100;
```

#### 不等值条件
```sql
SELECT * FROM products WHERE price != 50;
-- 或
SELECT * FROM products WHERE price <> 50;
```

#### 范围条件
```sql
-- BETWEEN (闭区间)
SELECT * FROM orders WHERE amount BETWEEN 100 AND 500;

-- 比较运算符
SELECT * FROM employees WHERE salary > 10000;
```

---

### 2. **多条件组合**
#### AND 逻辑
```sql
SELECT * FROM customers 
WHERE country = 'USA' AND age > 30;
```

#### OR 逻辑
```sql
SELECT * FROM products 
WHERE category = 'Electronics' OR price < 100;
```

#### NOT 逻辑
```sql
SELECT * FROM users WHERE NOT status = 'inactive';
```

#### 混合逻辑（使用括号控制优先级）
```sql
SELECT * FROM orders 
WHERE (status = 'Shipped' OR status = 'Processing')
AND total_amount > 1000;
```

---

### 3. **模糊匹配**
#### `LIKE` 通配符
```sql
-- % 匹配任意字符（含空）
SELECT * FROM books WHERE title LIKE 'Database%';

-- _ 匹配单个字符
SELECT * FROM users WHERE phone LIKE '138____1234';
```

#### `REGEXP` 正则表达式
```sql
-- 匹配以 A 或 B 开头的名字
SELECT * FROM employees WHERE name REGEXP '^[AB]';
```

---

### 4. **集合操作**
#### `IN` 匹配列表
```sql
SELECT * FROM products WHERE id IN (101, 205, 307);
```

#### `NOT IN` 排除列表
```sql
SELECT * FROM cities WHERE country NOT IN ('CN', 'US');
```

---

### 5. **空值检查**
```sql
-- 检查空值
SELECT * FROM students WHERE email IS NULL;

-- 检查非空值
SELECT * FROM students WHERE email IS NOT NULL;
```

---

### 6. **复杂条件**
#### 子查询条件
```sql
-- 单值子查询
SELECT * FROM orders 
WHERE customer_id = (SELECT id FROM customers WHERE name = 'Alice');

-- 多值子查询 (IN + 子查询)
SELECT * FROM products 
WHERE category_id IN (SELECT id FROM categories WHERE type = 'Digital');
```

#### `EXISTS` 条件
```sql
SELECT * FROM suppliers 
WHERE EXISTS (SELECT 1 FROM products WHERE supplier_id = suppliers.id);
```

#### JSON 字段查询
```sql
-- 查询 JSON 字段中的属性
SELECT * FROM orders 
WHERE JSON_EXTRACT(order_data, '$.status') = 'completed';
```

#### 函数/表达式条件
```sql
-- 使用函数
SELECT * FROM users WHERE YEAR(created_at) = 2023;

-- 计算表达式
SELECT * FROM items WHERE (price * quantity) > 1000;
```

---

### 7. **其他高级用法**
#### `CASE` 条件
```sql
SELECT *,
  CASE 
    WHEN score >= 90 THEN 'A'
    WHEN score >= 80 THEN 'B'
    ELSE 'C'
  END AS grade
FROM exams;
```

#### 位运算条件
```sql
-- 检查第 3 位是否为 1 (二进制)
SELECT * FROM settings WHERE flags & 4 = 4;
```

#### 全文搜索 (`MATCH AGAINST`)
```sql
SELECT * FROM articles 
WHERE MATCH(title, content) AGAINST('database optimization' IN BOOLEAN MODE);
```

---

### 📌 关键说明：
1. **优先级控制**：使用 `()` 明确组合条件（如 `(A OR B) AND C`）。
2. **性能注意**：避免在字段上使用函数（如 `YEAR(date_column) = 2023`），会导致索引失效。
3. **正则谨慎**：`REGEXP` 通常比 `LIKE` 慢，避免在大数据表使用。
4. **NULL 处理**：始终用 `IS NULL`/`IS NOT NULL`，而非 `= NULL`。

> 以上示例覆盖 MySQL 8.0 的主流条件写法，可根据实际场景组合使用。