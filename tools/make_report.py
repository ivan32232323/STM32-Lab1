# Генерує docs/report.md і docs/report.docx з одного джерела (далі LibreOffice -> PDF).
# Запуск з кореня репозиторію: python3 tools/make_report.py
import os, re
from docx import Document
from docx.shared import Pt, Cm
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_BREAK
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

R = 'https://github.com/ivan32232323/STM32-Lab1'
C = lambda sha: f'{R}/commit/{sha}'

# Хеші комітів беруться з git log за префіксом повідомлення коміту
import subprocess
def _sha(prefix):
    log = subprocess.run(['git', 'log', '--all', '--format=%h %s'], capture_output=True, text=True).stdout
    for line in log.splitlines():
        h, _, subj = line.partition(' ')
        if subj.startswith(prefix):
            return h
    return '???????'
_work = _sha('lab1: firmware')
H = {
    'build':      _sha('build: Docker toolchain'),
    'skeleton':   _work,
    'main':       _work,
    'wokwi':      _work,
    'components': _work,
    'prd':        _work,
    'shots':      _work,
}
STUDENT, GROUP, COURSE, TEACHER = 'Шведенко Іван Олексійович', 'ІО-32', '4', 'Гордієнко Н.Ю.'
VIDEO = '[посилання на відео буде додано]'

TITLE = [
    ('c', 'Міністерство освіти і науки України'),
    ('c', 'Національний технічний університет України'),
    ('c', '«Київський політехнічний інститут імені Ігоря Сікорського»'),
    ('gap', 1),
    ('c', 'Факультет інформатики та обчислювальної техніки'),
    ('gap', 1),
    ('c', 'Кафедра обчислювальної техніки'),
    ('gap', 2),
    ('cb', 'Лабораторна робота №1'),
    ('c', 'з дисципліни «Системне програмування»'),
    ('c', '«Налаштування середовища розробки та bring-up STM32-платформи»'),
    ('gap', 6),
    ('r', 'Виконав:'), ('r', f'студент {COURSE} курсу'), ('r', f'групи {GROUP}'), ('r', STUDENT),
    ('gap', 1),
    ('r', 'Перевірив:'), ('r', TEACHER),
    ('gap', 8),
    ('c', 'Київ — 2026'),
]

B = []
def h1(t): B.append(('h1', t))
def h2(t): B.append(('h2', t))
def p(t): B.append(('p', t))
def code(t): B.append(('code', t))
def img(path, w, cap): B.append(('img', path, w, cap))
def tbl(rows, widths=None): B.append(('tbl', rows, widths))
def bul(items): B.append(('bul', items))

B.append(('center', 'Лабораторна робота №1'))
B.append(('center', 'Налаштування середовища розробки та bring-up STM32-платформи'))
h1('1. Мета лабораторної')
p('Розібратися з повним циклом розробки під STM32: налаштувати збірку в Docker, запустити прошивку '
  'в симуляторі Wokwi і отримати вивід у UART. Перевірити роботу I²C, ADC, PWM і GPIO, а також описати '
  'вимоги до свого проєкту у вигляді PRD.')
p(f'Репозиторій: {R}')

h1('2. Розподіл обов\'язків')
p('Робота виконана одноосібно, таблиця не потрібна.')

h1('3. Процес виконання')
h2('3.1. Журнал роботи')
tbl([['Дата', 'Що зроблено'],
     ['06.11', 'Встановлено VS Code, плагін Wokwi, Git, Docker Desktop. Створено Dockerfile і кореневий Makefile'],
     ['06.11', 'Створено проєкт під Nucleo-C031C6: HAL, CMSIS, startup, linker, .ioc'],
     ['06.11', 'printf через USART2, ініціалізація I2C1, ADC1, TIM3, GPIO, I²C-скан, WHO_AM_I, вивід tick N'],
     ['06.11', 'Складено diagram.json, docs/hardware-components.md і docs/PRD.md'],
     ['06.11', 'Запуск у Wokwi, налагодження читання MPU-6050, захоплення I²C логічним аналізатором'],
     ['06.11', 'Скриншоти і звіт'],
     ], [2.0, 14.5])

h2('3.2. Docker-тулчейн')
p('Образ stm32-build на базі ubuntu:22.04 з arm-none-eabi-gcc. Збірка запускається командою make build '
  'і не залежить від налаштувань системи. На Windows без make можна запустити build.bat.')
code('make image   # docker build -t stm32-build docker/\nmake build   # docker run ... stm32-build make -C firmware all')
code('LD  build/firmware.elf\n   text    data     bss     dec     hex filename\n  25612     112    2320   28044    6d8c build/firmware.elf')
p(f'Коміт: {C(H["build"])}')

h2('3.3. STM32CubeMX')
tbl([['Периферія', 'Параметри', 'Виводи'],
     ['USART2', '115200 8N1', 'PA2 / PA3'],
     ['I2C1', 'Standard Mode, 100 kHz', 'PB8 (SCL) / PB9 (SDA)'],
     ['TIM3 CH1, CH2', 'PWM 50 Hz (Prescaler = 47, Period = 19999)', 'PA6 / PA7'],
     ['ADC1', 'IN0', 'PA0'],
     ['GPIO', 'кнопка MODE (pull-up), світлодіод LD4', 'PA10 / PA5'],
     ], [3.0, 8.5, 5.0])
p('SYSCLK = 48 MHz. Проєкт з тулчейном Makefile, файл .ioc збережено в репозиторії.')
p('Pinout:')
img('docs/screenshots/cubemx-pinout.png', 16.0, '')
p('Clock Configuration:')
img('docs/screenshots/cubemx-clock.png', 16.0, '')
p(f'Коміт: {C(H["skeleton"])}')

h2('3.4. Перший запуск у Wokwi')
p('printf перенаправлено в USART2 через функцію _write(). Налаштовано wokwi.toml і diagram.json. '
  'Розмір прошивки — 25 612 байт.')
p('Вивід у терміналі Wokwi після першого запуску:')
img('docs/screenshots/wokwi-uart-first-run.png', 15.5, '')
p(f'Коміт: {C(H["main"])}')

h2('3.5. Компоненти і datasheets')
p('У docs/hardware-components.md для кожного компонента вказано інтерфейс, адресу, ключові регістри '
  'і параметри з номером сторінки datasheet.')
img('docs/datasheets/mpu6050-rm-p45-who-am-i.png', 13.5, 'Джерело: [1], с. 45 — регістр WHO_AM_I')
img('docs/datasheets/mpu6050-rm-p40-pwr-mgmt1.png', 13.5, 'Джерело: [1], с. 40 — PWR_MGMT_1')
img('docs/datasheets/mpu6050-rm-p29-accel.png', 13.5, 'Джерело: [1], с. 29 — регістри акселерометра')
img('docs/datasheets/mpu6050-ps-p15-i2c-addr.png', 13.5, 'Джерело: [2], с. 15 — I²C-адреса і частота')
img('docs/datasheets/mpu6050-ps-p14-current.png', 13.5, 'Джерело: [2], с. 14 — струм споживання')
img('docs/datasheets/mpu6050-ps-p35-i2c-protocol.png', 13.5, 'Джерело: [2], с. 35 — протокол I²C')
p(f'Коміт: {C(H["components"])}')

h2('3.6. Сенсори і перевірка шин')
p('До diagram.json додано MPU-6050, потенціометр, два серво SG90, кнопку і логічний аналізатор. '
  'Кнопкою перемикається режим: MANUAL (серво від потенціометра) або MIRROR (серво повторюють нахил MPU-6050).')
p('Робота в режимі MIRROR:')
img('docs/screenshots/wokwi-mirror.png', 15.5, '')
p('Декодована транзакція WHO_AM_I (запис з логічного аналізатора Wokwi, wokwi.vcd):')
img('docs/screenshots/logic-i2c-whoami.png', 16.5, '')
p('PWM на виводі серво:')
img('docs/screenshots/logic-pwm.png', 16.5, '')
p(f'Коміт: {C(H["wokwi"])}')

h2('3.7. Проблеми та їх вирішення')
PROBS = [
 ('у Wokwi немає Nucleo-F411RE / F446RE',
  'у списку плат Wokwi є тільки Nucleo-C031C6 і Nucleo-L031K6.',
  'Wokwi симулює лише кілька плат STM32, лінійки F4 серед них немає.',
  'взяв Nucleo-C031C6, підключив HAL для STM32C0 і змінив у Makefile -mcpu на cortex-m0plus.', H['skeleton']),
 ('збірка падає на m-profile/cmsis_gcc_m.h: No such file or directory',
  'make build -> fatal error у cmsis_gcc.h.',
  'змішав версії CMSIS: cmsis_gcc.h був з CMSIS 6 і підключає m-profile/, а пакет HAL для C0 іде з CMSIS 5, де цієї папки немає.',
  'взяв папку CMSIS/Include з того ж пакета, що й HAL.', H['skeleton']),
 ('implicit declaration of function HAL_TIM_MspPostInit',
  'попередження при збірці в MX_TIM3_Init().',
  'функція визначена в stm32c0xx_hal_msp.c, а її прототип CubeMX кладе в main.h. У моєму main.h його не було.',
  'додав прототип функції в main.h.', H['main']),
 ('make build не працює на Windows',
  'у PowerShell немає make, а $(id -u) у Makefile не спрацьовує.',
  'команда id є тільки в Unix-оболонці. На Windows Docker Desktop і так створює файли від імені користувача, тому --user там не потрібен.',
  'для Windows прибрав --user у Makefile і додав build.bat.', H['build']),
 ('MPU-6050 — WHO_AM_I = 0x00',
  'I²C-скан бачить 0x68, але HAL_I2C_Mem_Read повертає HAL_ERROR (err = 0x20, timeout). Master_Transmit + Master_Receive дають те саме.',
  'у логічному аналізаторі видно, що після адреси читання 0xD1 немає тактів SCL. Модель I²C STM32C0 у Wokwi не тактує шину в режимі прийому, '
  'а запис (і тому скан) працює.',
  'зробив читання програмним I²C на тих самих пінах, SDA під час читання перемикаю на вхід. Після цього WHO_AM_I = 0x68.', H['main']),
]
for i, (t, s, r, f, sha) in enumerate(PROBS, 1):
    B.append(('prob', i, t, s, r, f, sha))

h1('4. Висновок')
p('Навчився:')
bul(['збирати прошивку STM32 в Docker однією командою make build;',
     'налаштовувати тактування через HAL: HSI48 без дільника дає SYSCLK = 48 МГц;',
     'рахувати Prescaler і Period таймера для PWM 50 Гц під серво (48 МГц / 48 = 1 МГц, період 20000 тактів);',
     'виводити printf в UART через _write() і перевіряти статус кожної ініціалізації HAL;',
     'знаходити в datasheet адресу, регістри і живлення датчика;',
     'перевіряти I²C-пристрої за адресою і WHO_AM_I та розбирати транзакцію в логічному аналізаторі.'])
p('Що зробив би інакше: одразу перевірив би, які плати є у Wokwi, і створив проєкт у CubeMX під Nucleo-C031C6; '
  'брав би HAL і CMSIS з одного пакета; раніше підключив би логічний аналізатор — проблему з I²C знайшов би швидше; '
  'комітив би частіше, після кожного кроку.')
p('Куди рухаюся в ЛР2: почати реалізацію вимог з PRD — читати гіроскоп, додати фільтр для кутів, '
  'виміряти затримку від руху датчика до руху серво.')

h1('5. Матеріали та посилання')
B.append(('ol', [
 'InvenSense. MPU-6000/MPU-6050 Register Map and Descriptions, RM-MPU-6000A-00, rev 4.2. 46 с.',
 'InvenSense. MPU-6000/MPU-6050 Product Specification, PS-MPU-6000A-00, rev 3.4, 19.08.2013. 52 с.',
 'STMicroelectronics. STM32C031x4/x6 Datasheet, DS13867. https://www.st.com/en/microcontrollers-microprocessors/stm32c031c6.html',
 'STMicroelectronics. RM0490 Reference Manual: STM32C0 series. https://www.st.com/en/microcontrollers-microprocessors/stm32c031c6.html#documentation',
 'STMicroelectronics. STM32 Nucleo-64 boards (MB1717), user manual UM2953. https://www.st.com/resource/en/user_manual/um2953-stm32-nucleo64-board-mb1717-stmicroelectronics.pdf',
 'TowerPro. SG90 Micro Servo datasheet.',
 'Wokwi. Документація: diagram.json, STM32 Nucleo-C031C6. https://docs.wokwi.com',
]))

h1('6. Відеозвіт')
p(f'Відео (2–3 хв): представлення, запуск прошивки у Wokwi і лог UART: {VIDEO}')

h1('7. PRD (Product Requirements Document)')
p(f'Окремий файл docs/PRD.md: {R}/blob/lab1/docs/PRD.md')
p('Тема — проєкт 6 «Motion-Mirroring Teleoperator»: два серво повторюють нахил руки, виміряний MPU-6050. '
  'У PRD: A — загальна ідея, B — 11 функціональних вимог FR-01…FR-11, C — датчики з інтерфейсами, '
  'обґрунтуванням і альтернативами, D — план тестування для кожної вимоги.')
p(f'Коміт: {C(H["prd"])}')

SHA = re.compile(r'^[0-9a-f]{7}$')

def md():
    o = ['---', 'title: "Лабораторна робота №1"', '---', '']
    for k, t in TITLE:
        if k == 'gap': o.append('')
        else: o.append((f'**{t}**' if k == 'cb' else t) + '  ')
    o += ['', '---', '']
    for b in B:
        k = b[0]
        if k == 'center': o += [f'### {b[1]}', '']
        elif k == 'h1': o += [f'# {b[1]}', '']
        elif k == 'h2': o += [f'## {b[1]}', '']
        elif k == 'p': o += [b[1], '']
        elif k == 'code': o += ['```', b[1], '```', '']
        elif k == 'img': o += [f'![]({os.path.relpath(b[1], "docs")})', ''] + ([b[3], ''] if b[3] else [])
        elif k == 'tbl':
            rows = b[1]
            o += ['| ' + ' | '.join(rows[0]) + ' |', '|' + '---|' * len(rows[0])]
            for r in rows[1:]:
                cells = []
                for x in r:
                    parts = [y.strip() for y in x.split(',')]
                    if all(SHA.match(y) for y in parts):
                        x = ', '.join(f'[{y}]({C(y)})' for y in parts)
                    cells.append(x)
                o.append('| ' + ' | '.join(cells) + ' |')
            o.append('')
        elif k == 'bul': o += [f'- {x}' for x in b[1]] + ['']
        elif k == 'ol': o += [f'{i}. {x}' for i, x in enumerate(b[1], 1)] + ['']
        elif k == 'prob':
            _, i, t, s, r, f, sha = b
            o += [f'Проблема {i}: {t}  ', f'Симптом: {s}  ', f'Причина: {r}  ', f'Фікс: {f}  ', f'Коміт: {C(sha)}', '']
    return '\n'.join(o)

doc = Document()
st = doc.styles['Normal']; st.font.name = 'Times New Roman'; st.font.size = Pt(14)
st.element.rPr.rFonts.set(qn('w:eastAsia'), 'Times New Roman')
st.paragraph_format.space_after = Pt(4); st.paragraph_format.line_spacing = 1.15
for s in doc.sections:
    s.left_margin, s.right_margin, s.top_margin, s.bottom_margin = Cm(2.5), Cm(1.5), Cm(2), Cm(2)

def runs(par, text, size=None):
    for part in re.split(r'(\*\*.*?\*\*|`.*?`)', text):
        if not part: continue
        if part.startswith('**'):
            r = par.add_run(part[2:-2]); r.bold = True
        elif part.startswith('`'):
            r = par.add_run(part[1:-1]); r.font.name = 'Consolas'; r.font.size = Pt(12); continue
        else:
            r = par.add_run(part)
        if size: r.font.size = Pt(size)

for k, t in TITLE:
    if k == 'gap':
        for _ in range(t): doc.add_paragraph().paragraph_format.space_after = Pt(0)
        continue
    par = doc.add_paragraph(); par.paragraph_format.space_after = Pt(0)
    par.alignment = WD_ALIGN_PARAGRAPH.RIGHT if k == 'r' else WD_ALIGN_PARAGRAPH.CENTER
    r = par.add_run(t); r.bold = (k == 'cb')
doc.add_paragraph().add_run().add_break(WD_BREAK.PAGE)

def heading(text, size):
    par = doc.add_paragraph(); par.paragraph_format.space_before = Pt(10); par.paragraph_format.keep_with_next = True
    r = par.add_run(text); r.bold = True; r.font.size = Pt(size)

for b in B:
    k = b[0]
    if k == 'center':
        par = doc.add_paragraph(); par.alignment = WD_ALIGN_PARAGRAPH.CENTER; par.add_run(b[1]).bold = True
    elif k == 'h1': heading(b[1], 15)
    elif k == 'h2': heading(b[1], 14)
    elif k == 'p':
        par = doc.add_paragraph(); runs(par, b[1])
    elif k == 'code':
        par = doc.add_paragraph(); par.paragraph_format.line_spacing = 1.0
        e = OxmlElement('w:shd'); e.set(qn('w:val'), 'clear'); e.set(qn('w:fill'), 'F2F2F2')
        par._p.get_or_add_pPr().append(e)
        r = par.add_run(b[1]); r.font.name = 'Consolas'; r.font.size = Pt(10)
    elif k == 'img':
        par = doc.add_paragraph(); par.alignment = WD_ALIGN_PARAGRAPH.CENTER; par.paragraph_format.keep_with_next = True
        par.add_run().add_picture(b[1], width=Cm(b[2]))
        if b[3]:
            doc.add_paragraph(b[3])
    elif k == 'tbl':
        rows, widths = b[1], b[2]
        t = doc.add_table(rows=len(rows), cols=len(rows[0])); t.style = 'Table Grid'
        t.autofit = False
        if widths:
            for j, w in enumerate(widths): t.columns[j].width = Cm(w)
        for i, row in enumerate(rows):
            for j, val in enumerate(row):
                cell = t.cell(i, j); cell.text = ''
                r = cell.paragraphs[0].add_run(val); r.font.size = Pt(12); r.bold = (i == 0)
                if widths: cell.width = Cm(widths[j])
        doc.add_paragraph()
    elif k in ('bul', 'ol'):
        for x in b[1]:
            par = doc.add_paragraph(style='List Bullet' if k == 'bul' else 'List Number'); runs(par, x)
    elif k == 'prob':
        _, i, t, s, r, f, sha = b
        par = doc.add_paragraph(); par.paragraph_format.space_before = Pt(8); par.paragraph_format.keep_with_next = True
        par.add_run(f'Проблема {i}: {t}')
        doc.add_paragraph(f'Симптом: {s}'); doc.add_paragraph(f'Причина: {r}'); doc.add_paragraph(f'Фікс: {f}'); doc.add_paragraph(f'Коміт: {C(sha)}')

open('docs/report.md', 'w', encoding='utf-8').write(md())
doc.core_properties.author = STUDENT
doc.core_properties.last_modified_by = STUDENT
doc.save('docs/report.docx')
print('ok')
