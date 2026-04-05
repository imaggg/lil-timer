# Darkroom Timer for Lilka v2

Custom firmware for [Lilka v2](https://github.com/and3rson/lilka) — a darkroom wet process timer for film and paper development.

**Red-on-black only** display — safe for use under darkroom safelight conditions.

## Features

- **3 configurable presets** with up to 10 steps each (DEV, STOP, FIX, WASH, etc.)
- **Dmax Test** — dedicated stopwatch for determining paper development time
- **Auto-advance** between steps with audio cues
- **Sound alerts** — start, 30s marks, last 10s countdown, step transitions
- **WiFi web interface** — edit presets and settings from your phone/laptop
- **Bilingual UI** — English / Ukrainian
- **Adjustable brightness & volume**

## Controls

| Button | Function |
|--------|----------|
| UP/DOWN | Navigate |
| A | Confirm / Start / Resume |
| B | Back / Pause |
| L/R | Adjust values |
| START | Skip step / Edit preset name |
| C/D | Add / Delete step (editor) |

## Build

Requires [PlatformIO](https://platformio.org/).

```
pio run -e lilka_v2
pio run -e lilka_v2 -t upload
```

## License

MIT

---

# Таймер для фотолабораторії — Lilka v2

Кастомна прошивка для [Lilka v2](https://github.com/and3rson/lilka) — таймер мокрих процесів для проявки плівки та паперу.

Дисплей **червоний на чорному** — безпечно для роботи з лабораторним освітленням.

## Можливості

- **3 пресети** з до 10 кроків кожен (DEV, STOP, FIX, WASH, тощо)
- **Dmax Тест** — секундомір для визначення часу проявки паперу
- **Автоперехід** між кроками зі звуковими сигналами
- **Звукові сповіщення** — старт, позначки 30с, останні 10с, переходи
- **WiFi веб-інтерфейс** — редагування пресетів і налаштувань з телефону/ноутбука
- **Двомовний інтерфейс** — English / Українська
- **Регулювання яскравості та гучності**

## Керування

| Кнопка | Функція |
|--------|---------|
| UP/DOWN | Навігація |
| A | Підтвердити / Старт / Продовжити |
| B | Назад / Пауза |
| L/R | Змінити значення |
| START | Пропустити крок / Редагувати назву |
| C/D | Додати / Видалити крок (редактор) |

## Збірка

Потрібен [PlatformIO](https://platformio.org/).

```
pio run -e lilka_v2
pio run -e lilka_v2 -t upload
```
