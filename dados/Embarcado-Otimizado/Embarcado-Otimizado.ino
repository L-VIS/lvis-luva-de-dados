/**
 * @file lvis_luva_vr.ino
 * @brief Firmware da luva VR do L-VIS – leitura de sensores flex e IMU MPU6050.
 * 
 * Lê 5 sensores flex (polegar, indicador, médio, anelar, mínimo),
 * calcula ângulos de flexão por regressão linear,
 * e obtém orientação da mão (roll, pitch, yaw) via MPU6050.
 * 
 * Comunicação serial a 4800 bauds no formato:
 * roll,pitch,yaw,polegar,indicador,medio,anelar,minimo\n
 * 
 * @note Os valores de calibração estática (aberto/fechado) são definidos
 *       por macros. A calibração automática em tempo de execução
 *       pode sobrescrevê-los (mas aqui está desativada por padrão).
 */

#include <MPU6050_tockn.h>
#include <Wire.h>

// ========================================================================
//  MACROS DE CALIBRAÇÃO ESTÁTICA (valores brutos do ADC para cada dedo)
// ========================================================================

// --- Polegar (porta digital? Verifique!) ---
#define CAL_ABERTO_POLEGAR   0
#define CAL_FECHADO_POLEGAR  1

// --- Indicador (A0) ---
#define CAL_ABERTO_INDICADOR   450
#define CAL_FECHADO_INDICADOR  600

// --- Médio (A1) ---
#define CAL_ABERTO_MEDIO   550
#define CAL_FECHADO_MEDIO  617

// --- Anelar (A2) ---
#define CAL_ABERTO_ANELAR   575
#define CAL_FECHADO_ANELAR  657

// --- Mínimo (A3) ---
#define CAL_ABERTO_MINIMO   571
#define CAL_FECHADO_MINIMO  596

// ========================================================================
//  ESTRUTURA DE DADOS PARA UM DEDO
// ========================================================================

/**
 * @brief Representa um dedo com seus parâmetros de calibração e estado.
 */
struct Dedo {
  uint8_t porta;          //!< Pino analógico (ou digital para polegar)
  String  nome;           //!< Nome do dedo (para depuração)
  int     leitura;        //!< Último valor lido (ADC)
  int     angulo;         //!< Ângulo estimado (0° = aberto, 90° = fechado)
  int     aberto;         //!< Valor calibrado para o dedo esticado
  int     fechado;        //!< Valor calibrado para o dedo flexionado
};

// ========================================================================
//  OBJETOS GLOBAIS E INSTÂNCIAS
// ========================================================================

MPU6050 mpu6050(Wire);
Dedo dedos[5];   // Índices: 0=polegar, 1=indicador, 2=médio, 3=anelar, 4=mínimo

// ========================================================================
//  PROTÓTIPOS DAS FUNÇÕES
// ========================================================================

int  calcularAngulo(int leitura, int fechado, int aberto, bool saturar = true);
void lerSensores();
void calcularAngulosDedos();
void imprimirDados(char modo = 'v');
void calibrarDedos(int numeroAmostras = 1, int tempoEspera = 13000);

// ========================================================================
//  SETUP
// ========================================================================

void setup() {
  // --- Inicialização da comunicação serial ---
  Serial.begin(4800);

  // --- Inicialização do barramento I2C (para o MPU6050) ---
  Wire.begin();

  // --- Inicialização do MPU6050 ---
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);   // Calibração do giroscópio (demora alguns segundos)

  // --- Configuração dos dedos: portas, nomes e calibração estática ---
  dedos[0].porta   = 3;    // ATENÇÃO: esta porta é digital! Provavelmente deveria ser A3 (analógica)
  dedos[0].nome    = "polegar";
  dedos[0].aberto  = CAL_ABERTO_POLEGAR;
  dedos[0].fechado = CAL_FECHADO_POLEGAR;

  dedos[1].porta   = A0;
  dedos[1].nome    = "indicador";
  dedos[1].aberto  = CAL_ABERTO_INDICADOR;
  dedos[1].fechado = CAL_FECHADO_INDICADOR;

  dedos[2].porta   = A1;
  dedos[2].nome    = "medio";
  dedos[2].aberto  = CAL_ABERTO_MEDIO;
  dedos[2].fechado = CAL_FECHADO_MEDIO;

  dedos[3].porta   = A2;
  dedos[3].nome    = "anelar";
  dedos[3].aberto  = CAL_ABERTO_ANELAR;
  dedos[3].fechado = CAL_FECHADO_ANELAR;

  dedos[4].porta   = A3;
  dedos[4].nome    = "minimo";
  dedos[4].aberto  = CAL_ABERTO_MINIMO;
  dedos[4].fechado = CAL_FECHADO_MINIMO;

  // Define o modo de cada pino (entrada)
  for (int i = 0; i < 5; i++) {
    pinMode(dedos[i].porta, INPUT);
  }

  // --- Calibração automática (com apenas 1 amostra, conforme solicitado) ---
  // Nota: o ideal seria usar mais amostras para maior precisão.
  calibrarDedos(1);
}

// ========================================================================
//  LOOP PRINCIPAL
// ========================================================================

void loop() {
  // 1. Atualiza dados da IMU (roll, pitch, yaw)
  mpu6050.update();

  // 2. Lê os sensores flex
  lerSensores();

  // 3. Converte leituras brutas em ângulos
  calcularAngulosDedos();

  // 4. Envia os dados pela serial
  imprimirDados();   // modo 'v' (ângulos) por padrão

  // 5. Aguarda o próximo ciclo (taxa de atualização ~20 Hz)
  delay(50);
}

// ========================================================================
//  IMPLEMENTAÇÃO DAS FUNÇÕES
// ========================================================================

/**
 * @brief Lê os valores dos sensores flex de todos os dedos.
 * 
 * O polegar (índice 0) é lido com digitalRead (pino digital 3),
 * os demais com analogRead (portas A0..A3).
 */
void lerSensores() {
  // Polegar: leitura digital (0 ou 1) – provável erro de projeto
  dedos[0].leitura = digitalRead(dedos[0].porta);

  // Demais dedos: leitura analógica (0..1023)
  for (int i = 1; i < 5; i++) {
    dedos[i].leitura = analogRead(dedos[i].porta);
  }
}

/**
 * @brief Calcula o ângulo de cada dedo usando a função de ajuste linear.
 */
void calcularAngulosDedos() {
  for (int i = 0; i < 5; i++) {
    dedos[i].angulo = calcularAngulo(dedos[i].leitura,
                                     dedos[i].fechado,
                                     dedos[i].aberto,
                                     true);   // saturação ativa
  }
}

/**
 * @brief Estima o ângulo de flexão de um único dedo.
 * 
 * @param leitura   Valor lido do sensor (ADC ou digital)
 * @param fechado   Valor calibrado para o dedo fechado
 * @param aberto    Valor calibrado para o dedo aberto
 * @param saturar   Se true, limita o resultado entre 0 e 90 graus
 * @return int      Ângulo estimado em graus (0 = aberto, 90 = fechado)
 */
int calcularAngulo(int leitura, int fechado, int aberto, bool saturar) {
  // Interpolação linear: y = 90 * (x - x0) / (xf - x0)
  int angulo = 90 * (leitura - aberto) / (fechado - aberto);

  if (saturar) {
    if (angulo > 90)  angulo = 90;
    if (angulo < 0)   angulo = 0;
  }
  return angulo;
}

/**
 * @brief Envia os dados pela porta serial no formato esperado pelo Unity.
 * 
 * @param modo 'v' = envia ângulos (0..90), qualquer outro = envia leituras brutas (ADC)
 */
void imprimirDados(char modo) {
  // Envia orientação (roll, pitch, yaw) com 0 casas decimais
  Serial.print(mpu6050.getAngleX(), 0);
  Serial.print(',');
  Serial.print(mpu6050.getAngleY(), 0);
  Serial.print(',');
  Serial.print(mpu6050.getAngleZ(), 0);
  Serial.print(',');

  // Envia dados de cada dedo (separados por vírgula)
  for (int i = 0; i < 5; i++) {
    int valor = (modo == 'v') ? dedos[i].angulo : dedos[i].leitura;
    Serial.print(valor);
    if (i < 4) Serial.print(',');
  }
  Serial.println();   // finaliza com nova linha
}

/**
 * @brief Calibração automática em tempo de execução.
 * 
 * Pede ao usuário que feche e abra a mão, coletando amostras.
 * Os valores médios são armazenados nos campos 'fechado' e 'aberto'
 * de cada dedo, sobrescrevendo a calibração estática.
 * 
 * @param numeroAmostras  Quantas leituras serão feitas para cada posição
 * @param tempoEspera     Tempo (ms) para o usuário mudar de posição
 */
void calibrarDedos(int numeroAmostras, int tempoEspera) {
  // Zera os acumuladores para evitar lixo
  for (int i = 0; i < 5; i++) {
    dedos[i].fechado = 0;
    dedos[i].aberto  = 0;
  }

  Serial.println("\n=== INÍCIO DA CALIBRAÇÃO AUTOMÁTICA ===");
  Serial.println("Feche a mão (dedos flexionados) e aguarde...");
  delay(tempoEspera);

  Serial.println("Coletando amostras com a mão fechada...");
  for (int amostra = 0; amostra < numeroAmostras; amostra++) {
    lerSensores();
    for (int i = 0; i < 5; i++) {
      dedos[i].fechado += dedos[i].leitura;   // soma bruta
    }
    delay(50);
  }
  // Calcula a média (divisão pelo número de amostras)
  for (int i = 0; i < 5; i++) {
    dedos[i].fechado /= numeroAmostras;
  }

  Serial.println("Agora abra a mão (dedos esticados) e aguarde...");
  delay(tempoEspera);

  Serial.println("Coletando amostras com a mão aberta...");
  for (int amostra = 0; amostra < numeroAmostras; amostra++) {
    lerSensores();
    for (int i = 0; i < 5; i++) {
      dedos[i].aberto += dedos[i].leitura;
    }
    delay(50);
  }
  for (int i = 0; i < 5; i++) {
    dedos[i].aberto /= numeroAmostras;
  }

  // Exibe os novos valores de calibração
  Serial.println("\n=== NOVOS VALORES DE CALIBRAÇÃO ===");
  for (int i = 0; i < 5; i++) {
    Serial.print(dedos[i].nome);
    Serial.print(" => fechado: ");
    Serial.print(dedos[i].fechado);
    Serial.print(", aberto: ");
    Serial.println(dedos[i].aberto);
  }
  Serial.println("=== CALIBRAÇÃO CONCLUÍDA ===\n");
}