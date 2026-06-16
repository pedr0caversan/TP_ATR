import math
import pygame
import os

from MQTTInterface import MQTTInterface

os.environ['SDL_VIDEO_WINDOW_POS'] = "75,630"


class RemoteOperationInterface:
    def __init__(self) -> None:
        pygame.init()
        pygame.font.init()

        self.screen_width = 1750
        self.screen_height = 400
        self.screen = pygame.display.set_mode((self.screen_width, self.screen_height))
        pygame.display.set_caption("Interface de Controle Remoto")
        self.clock = pygame.time.Clock()
        self.running = True

        # Estado do robô
        self.mode = "MANUAL"

        # Pegar do mqtt, talvez a velocidade eu tenha que derivar
        self.position_x = 0.0
        self.speed = 0.0
        self.lidar_distance = 5.0
        self.is_inspecting = False

        self.speed_setpoint = 0.0
        self.last_command = "Nenhum comando enviado"

        # Enquanto não recebe dados verdadeiros
        self.speed = self.speed_setpoint

        self.position_history = [self.position_x]
        self.velocity_history = [self.speed]
        self.lidar_history = [self.lidar_distance]
        self.max_history = 220

        self.font = pygame.font.SysFont(None, 24)
        self.big_font = pygame.font.SysFont(None, 34, bold=True)
        self.FPS = 60

        # MQTT
        self.mqtt = MQTTInterface(actuator_callback=self.update_data_colector_variables)
        self.mqtt.connect()

    def update_data_colector_variables(self, data):
        """Callback chamado ao receber dados MQTT de múltiplos tópicos.

        Para o tópico `atr/telemetria/log`, atualiza diretamente a posição e o LIDAR.
        """
        if not isinstance(data, dict):
            return

        topic_type = data.get("type")
        payload = data.get("payload")

        if topic_type == "telemetria":
            if isinstance(payload, dict):
                x = payload.get("x")
                y = payload.get("y")
            else:
                x = None
                y = None

            if x is not None:
                try:
                    self.position_x = float(x)
                except Exception:
                    pass

            if y is not None:
                try:
                    self.lidar_distance = float(y)
                except Exception:
                    pass

        elif topic_type == "velocidade":
            try:
                self.speed = float(payload)
            except Exception:
                pass

        elif topic_type == "inspecao":
            try:
                self.is_inspecting = bool(payload)
            except Exception:
                pass

    def act_upon_pressed_keys(self) -> None:
        keys = pygame.key.get_pressed()

        if keys[pygame.K_a]:
            self.mode = "AUTOMÁTICO"
            self.last_command = "Ativou modo automático"
            self.speed_setpoint = 10.0
        elif keys[pygame.K_m]:
            self.mode = "MANUAL"
            self.last_command = "Ativou modo manual"
            self.speed_setpoint = 0.0

        if self.mode == "MANUAL":
            if keys[pygame.K_LEFT]:
                self.speed_setpoint = -15.0
                self.last_command = "Mandou o robô ir para a esquerda"
            elif keys[pygame.K_RIGHT]:
                self.speed_setpoint = 15.0
                self.last_command = "Mandou o robô ir para a direita"
            elif keys[pygame.K_s]:
                self.speed_setpoint = 0.0
                self.last_command = "Mandou o robô parar"

    def update_state(self) -> None:
        self.position_history.append(self.position_x)
        self.velocity_history.append(self.speed)
        self.lidar_history.append(self.lidar_distance)

        if len(self.position_history) > self.max_history:
            self.position_history.pop(0)
            self.velocity_history.pop(0)
            self.lidar_history.pop(0)

    def draw_text(self, text: str, x: int, y: int, color=(255, 255, 255)) -> None:
        surface = self.font.render(text, True, color)
        self.screen.blit(surface, (x, y))

    def draw_graph(self, values: list[float], rect: pygame.Rect, color: tuple[int, int, int], label: str, min_value: float, max_value: float) -> None:
        pygame.draw.rect(self.screen, (20, 20, 60), rect)
        pygame.draw.rect(self.screen, (80, 80, 140), rect, 2)

        label_surface = self.font.render(label, True, (255, 255, 255))
        self.screen.blit(label_surface, (rect.x + 8, rect.y + 8))

        if len(values) < 2:
            return

        graph_left = rect.x + 8
        graph_top = rect.y + 32
        graph_width = rect.width - 16
        graph_height = rect.height - 40

        pygame.draw.line(self.screen, (100, 100, 180), (graph_left, graph_top + graph_height // 2), (graph_left + graph_width, graph_top + graph_height // 2), 1)

        scaled = []
        range_value = max_value - min_value if max_value != min_value else 1.0
        for value in values:
            normalized = (value - min_value) / range_value
            sample_y = graph_top + graph_height - int(normalized * graph_height)
            scaled.append(sample_y)

        step = graph_width / (len(scaled) - 1)
        points = []
        for index, sample_y in enumerate(scaled):
            points.append((graph_left + index * step, sample_y))

        if len(points) >= 2:
            pygame.draw.lines(self.screen, color, False, points, 2)

        min_label = self.font.render(f"{min_value:.0f}", True, (180, 180, 180))
        max_label = self.font.render(f"{max_value:.0f}", True, (180, 180, 180))
        self.screen.blit(min_label, (rect.x + rect.width - 45, rect.y + rect.height - 22))
        self.screen.blit(max_label, (rect.x + rect.width - 45, rect.y + 10))

    def draw_interface(self) -> None:
        self.screen.fill((12, 12, 30))

        margin = 20
        left_panel_width = max(400, int(self.screen_width * 0.62))
        right_panel_x = left_panel_width + margin
        right_panel_width = max(260, self.screen_width - right_panel_x - margin)
        graph_width = left_panel_width - 2 * margin
        graph_height = max(150, int((self.screen_height - 3 * margin) / 2))

        text_x = right_panel_x + 10
        text_y = margin
        line_height = 32
        x_offset = 20
        circle_offset = 8

        self.draw_text("Inspecionando falha:", text_x, text_y, (255, 255, 255))
        text_width, text_height = self.font.size("Inspecionando falha:")
        if self.is_inspecting:
            pygame.draw.circle(self.screen, (0,255,0), (text_x+text_width+x_offset,text_y+circle_offset), 10, 0)
        else:
            pygame.draw.circle(self.screen, (255,0,0), (text_x+text_width+x_offset,text_y+circle_offset), 10, 0)
        text_y += line_height

        self.draw_text(f"Modo: {self.mode}", text_x, text_y)
        text_y += line_height
        self.draw_text(f"Posição: {self.position_x:.1f} m", text_x, text_y)
        text_width, text_height = self.font.size(f"Posição: {self.position_x:.1f}")
        self.draw_text(f"Velocidade: {self.speed:.1f} m/s", text_x+text_width+x_offset*3, text_y)
        text_y += line_height
        self.draw_text(f"LIDAR: {self.lidar_distance:.1f} m", text_x, text_y)
        text_y += line_height
        self.draw_text("Último comando:", text_x, text_y)
        text_width, text_height = self.font.size(f"Último comando:")
        self.draw_text(self.last_command, text_x+text_width+x_offset*2, text_y+circle_offset/2, (200, 200, 120))
        text_y += line_height * 2

        self.draw_text("Comandos disponíveis", text_x, text_y, (140, 220, 140))
        text_width, text_height = self.font.size(f"Comandos disponíveis")
        if self.mode == "AUTOMÁTICO":
            self.draw_text("Opções abaixo desabilitadas.", text_x+text_width*1.5, text_y, (220, 140, 140))
        else:
            self.draw_text("Opções abaixo habilitadas.", text_x+text_width*1.5, text_y, (140, 220, 140))
        text_y += line_height
        self.draw_text("A : Ativar modo automático", text_x, text_y)
        self.draw_text("<- : Esquerda (manual)", text_x+text_width*1.5, text_y)
        text_y += line_height
        self.draw_text("M : Ativar modo manual", text_x, text_y)
        self.draw_text("-> : Direita (manual)", text_x+text_width*1.5, text_y)
        text_y += line_height
        self.draw_text("S : Parar (manual)", text_x+text_width*1.5, text_y)

        velocity_rect = pygame.Rect(margin, margin, graph_width, graph_height)
        lidar_rect = pygame.Rect(margin, margin + graph_height + margin, graph_width, graph_height)

        # self.draw_graph(self.position_history, position_rect, (80, 220, 120), "Posição (m)", 0, self.screen_width)
        self.draw_graph(self.velocity_history, velocity_rect, (240, 180, 40), "Velocidade (m/s)", -5, 5)
        self.draw_graph(self.lidar_history, lidar_rect, (180, 100, 240), "Distância LIDAR (m)", 3, 7)

    def run(self) -> None:
        while self.running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False

            self.act_upon_pressed_keys()
            self.update_state()
            self.draw_interface()

            self.mqtt.publish_setpoint(self.speed_setpoint)

            pygame.display.flip()
            self.clock.tick(self.FPS)

        pygame.quit()


if __name__ == "__main__":
    interface = RemoteOperationInterface()
    interface.run()
