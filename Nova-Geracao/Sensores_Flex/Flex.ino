/*
 * =====================================================================
 * LUVA VR – LEITURA DOS SENSORES FLEXÍVEIS (4 DEDOS)
 * Laboratório L-VIS / UnB
 * 
 * Versão: 3.0 – SOMENTE SENSORES FLEX (sem MPU6050)
 * Filtro de média móvel (N=6) e calibração automática
 * Baseada nos dados de resistência medidos em laboratório:
 *   - Indicador: R_0°=66 kΩ, R_90°=87 kΩ  → R_pull=82 kΩ
 *   - Médio:    R_0°=64 kΩ, R_90°=88 kΩ  → R_pull=82 kΩ
 *   - Anelar:   R_0°=28 kΩ, R_90°=31 kΩ  → R_pull=33 kΩ
 *   - Mindinho: R_0°=29 kΩ, R_90°=32 kΩ  → R_pull=33 kΩ
 * 
 * Pinos (ESP32):
 *   - Indicador: VP  (GPIO 36)
 *   - Médio:     VN  (GPIO 39)
 *   - Anelar:    D33 (GPIO 33)
 *   - Mindinho:  D34 (GPIO 34)
 * =====================================================================
 */

// ============================
// 1. PINOS DOS SENSORES (4 DEDOS)
// ============================
#define PIN_INDICADOR  36   // VP
#define PIN_MEDIO      39   // VN
#define PIN_ANELAR     33   // D33
#define PIN_MINDINHO   34   // D34

// ============================
// 2. VALORES DE CALIBRAÇÃO (ADC) – CALCULADOS A PARTIR DOS DADOS REAIS
// ============================
// Fórmula: ADC = (R_pull / (R_sensor + R_pull)) * 4095
// Vref = 3.3V, ADC de 12 bits (0–4095)

// --- Indicador (R_0°=66k, R_90°=87k, R_pull=82k) ---
#define ADC_INDICADOR_ABERTO    (int)((82000.0 / (66000.0 + 82000.0)) * 4095.0)  // ≈ 1818
#define ADC_INDICADOR_FECHADO   (int)((82000.0 / (87000.0 + 82000.0)) * 4095.0)  // ≈ 1946

// --- Médio (R_0°=64k, R_90°=88k, R_pull=82k) ---
#define ADC_MEDIO_ABERTO        (int)((82000.0 / (64430.0 + 82000.0)) * 4095.0)  // ≈ 1838
#define ADC_MEDIO_FECHADO       (int)((82000.0 / (87970.0 + 82000.0)) * 4095.0)  // ≈ 1949

// --- Anelar (R_0°=28k, R_90°=31k, R_pull=33k) ---
#define ADC_ANELAR_ABERTO       (int)((33000.0 / (28430.0 + 33000.0)) * 4095.0)  // ≈ 2030
#define ADC_ANELAR_FECHADO      (int)((33000.0 / (31130.0 + 33000.0)) * 4095.0)  // ≈ 2107

// --- Mindinho (R_0°=29k, R_90°=32k, R_pull=33k) ---
#define ADC_MINDINHO_ABERTO     (int)((33000.0 / (28700.0 + 33000.0)) * 4095.0)  // ≈ 2041
#define ADC_MINDINHO_FECHADO    (int)((33000.0 / (32000.0 + 33000.0)) * 4095.0)  // ≈ 2125

// ============================
// 3. ESTRUTURA DE DADOS (4 DEDOS)
// ============================
struct Dedo {
  uint8_t pino;          // pino analógico
  char    nome[12];      // nome do dedo
  int     adcAberto;     // ADC quando aberto (limite inferior)
  int     adcFechado;    // ADC quando fechado (limite superior)
  int     leituraBruta;  // última leitura direta do ADC
  int     leituraFiltrada; // leitura após média móvel
  int     angulo;        // ângulo estimado (0–90°)
};

// ============================
// 4. INSTÂNCIAS GLOBAIS
// ============================
Dedo dedos[4];

// ============================
// 5. FILTRO DE MÉDIA MÓVEL (N=6)
// ============================
const int N = 6;          // número de amostras na janela
int buffer[4][N];         // buffer para cada dedo (4 × N)
int indices[4] = {0,0,0,0};
long soma[4] = {0,0,0,0};

// ============================
// 6. PROTÓTIPOS DAS FUNÇÕES
// ============================
void lerSensoresFlex();
void aplicarFiltroMediaMovel();
void calcularAngulos();
void imprimirDados();
void calibrarDedos();

// ============================
// 7. SETUP
// ============================
void setup() {
  Serial.begin(9600);  // Baudrate aumentado para 9600

  // --- Configura os 4 dedos ---
  dedos[0].pino = PIN_INDICADOR;
  strcpy(dedos[0].nome, "indicador");
  dedos[0].adcAberto  = ADC_INDICADOR_ABERTO;
  dedos[0].adcFechado = ADC_INDICADOR_FECHADO;

  dedos[1].pino = PIN_MEDIO;
  strcpy(dedos[1].nome, "medio");
  dedos[1].adcAberto  = ADC_MEDIO_ABERTO;
  dedos[1].adcFechado = ADC_MEDIO_FECHADO;

  dedos[2].pino = PIN_ANELAR;
  strcpy(dedos[2].nome, "anelar");
  dedos[2].adcAberto  = ADC_ANELAR_ABERTO;
  dedos[2].adcFechado = ADC_ANELAR_FECHADO;

  dedos[3].pino = PIN_MINDINHO;
  strcpy(dedos[3].nome, "mindinho");
  dedos[3].adcAberto  = ADC_MINDINHO_ABERTO;
  dedos[3].adcFechado = ADC_MINDINHO_FECHADO;

  // Inicializa os buffers com zeros (para as primeiras leituras)
  for (int d = 0; d < 4; d++) {
    for (int i = 0; i < N; i++) {
      buffer[d][i] = 0;
    }
  }

  // --- Calibração automática (usa os limites reais) ---
  calibrarDedos();
}

// ============================
// 8. LOOP PRINCIPAL
// ============================
void loop() {
  // 1) Lê os sensores flexíveis (bruto)
  lerSensoresFlex();

  // 2) Aplica o filtro de média móvel (N=6)
  aplicarFiltroMediaMovel();

  // 3) Calcula os ângulos a partir dos valores filtrados
  calcularAngulos();

  // 4) Envia os dados pela serial
  imprimirDados();

  // 5) Aguarda 50ms (≈ 20 Hz)
  delay(50);
}

// ============================
// 9. FUNÇÕES DE LEITURA E FILTRAGEM
// ============================

/**
 * @brief Lê o valor bruto do ADC de cada dedo
 */
void lerSensoresFlex() {
  for (int i = 0; i < 4; i++) {
    dedos[i].leituraBruta = analogRead(dedos[i].pino);
  }
}

/**
 * @brief Aplica o filtro de média móvel (janela N=6) a cada dedo
 */
void aplicarFiltroMediaMovel() {
  for (int d = 0; d < 4; d++) {
    int valor = dedos[d].leituraBruta;

    // Subtrai o valor mais antigo do buffer
    soma[d] -= buffer[d][indices[d]];

    // Armazena o novo valor
    buffer[d][indices[d]] = valor;

    // Adiciona o novo valor à soma
    soma[d] += valor;

    // Avança o índice circularmente
    indices[d] = (indices[d] + 1) % N;

    // Calcula a média (divisão inteira)
    dedos[d].leituraFiltrada = soma[d] / N;
  }
}

// ============================
// 10. CÁLCULO DOS ÂNGULOS (0°–90°)
// ============================

/**
 * @brief Calcula o ângulo de cada dedo com base na leitura filtrada
 * 
 * A faixa de ADC é mapeada linearmente para 0° (aberto) e 90° (fechado).
 * O valor é saturado automaticamente para evitar ultrapassar os limites.
 */
void calcularAngulos() {
  for (int i = 0; i < 4; i++) {
    int x = dedos[i].leituraFiltrada;
    int xa = dedos[i].adcAberto;
    int xf = dedos[i].adcFechado;

    // Evita divisão por zero
    if (xf == xa) {
      dedos[i].angulo = 0;
      continue;
    }

    // Mapeamento linear: angulo = (leitura - aberto) / (fechado - aberto) * 90
    long angulo = 90L * (x - xa) / (xf - xa);

    // Saturação
    if (angulo > 90) angulo = 90;
    if (angulo < 0)  angulo = 0;

    dedos[i].angulo = (int)angulo;
  }
}

// ============================
// 11. SAÍDA DE DADOS (SERIAL)
// ============================

/**
 * @brief Envia os dados no formato CSV pela serial
 * Formato: indicador,medio,anelar,mindinho
 */
void imprimirDados() {
  for (int i = 0; i < 4; i++) {
    Serial.print(dedos[i].angulo);
    if (i != 3) Serial.print(',');
  }
  Serial.println();
}

// ============================
// 12. CALIBRAÇÃO AUTOMÁTICA
// ============================

/**
 * @brief Calibração automática baseada nos limites reais medidos
 * 
 * O usuário deve fechar e abrir a mão durante a calibração.
 * Os valores de ADC_ABERTO e ADC_FECHADO são ajustados dinamicamente
 * a partir dos limites reais definidos nas constantes.
 */
void calibrarDedos() {
  Serial.println("\n=== CALIBRAÇÃO AUTOMÁTICA ===");
  Serial.println("1) Feche a mão (punho) e aguarde...");
  delay(3000);

  // --- FASE 1: DEDOS FECHADOS ---
  long somaFechado[4] = {0,0,0,0};
  int amostras = 20;
  for (int a = 0; a < amostras; a++) {
    lerSensoresFlex();
    for (int i = 0; i < 4; i++) {
      somaFechado[i] += dedos[i].leituraBruta;
    }
    delay(50);
  }
  for (int i = 0; i < 4; i++) {
    int media = somaFechado[i] / amostras;
    // Atualiza o limite fechado, mas não ultrapassa o valor real
    if (media > dedos[i].adcFechado) {
      dedos[i].adcFechado = media;
    }
    // Garante que o limite fechado não ultrapasse 4095
    if (dedos[i].adcFechado > 4095) dedos[i].adcFechado = 4095;
  }

  Serial.println("2) Abra a mão (dedos esticados) e aguarde...");
  delay(3000);

  // --- FASE 2: DEDOS ABERTOS ---
  long somaAberto[4] = {0,0,0,0};
  for (int a = 0; a < amostras; a++) {
    lerSensoresFlex();
    for (int i = 0; i < 4; i++) {
      somaAberto[i] += dedos[i].leituraBruta;
    }
    delay(50);
  }
  for (int i = 0; i < 4; i++) {
    int media = somaAberto[i] / amostras;
    if (media < dedos[i].adcAberto) {
      dedos[i].adcAberto = media;
    }
    if (dedos[i].adcAberto < 0) dedos[i].adcAberto = 0;
  }

  // --- EXIBE OS VALORES CALIBRADOS ---
  Serial.println("\n--- Valores calibrados (aberto, fechado) ---");
  for (int i = 0; i < 4; i++) {
    Serial.print(dedos[i].nome);
    Serial.print(": ");
    Serial.print(dedos[i].adcAberto);
    Serial.print(", ");
    Serial.println(dedos[i].adcFechado);
  }
  Serial.println("============================================\n");
}