import math
import pygame


class RemoteOperationInterface:
    def __init__(self) -> None:
        pygame.init()
        pygame.font.init()

        self.screen_width = 1280
        self.screen_height = 720
        self.screen = pygame.display.set_mode((self.screen_width, self.screen_height))
        pygame.display.set_caption("Interface de Controle Remoto")
        self.clock = pygame.time.Clock()
        self.running = True

        # Estado do robô
        self.mode = "MANUAL"
        self.position_x = 100.0
        self.velocity_x = 0.0
        self.lidar_distance = 100.0
        self.last_command = "Nenhum comando enviado"

        self.position_history = [self.position_x]
        self.velocity_history = [self.velocity_x]
        self.lidar_history = [self.lidar_distance]
        self.max_history = 220

        self.font = pygame.font.SysFont(None, 24)
        self.big_font = pygame.font.SysFont(None, 34, bold=True)
        self.FPS = 60

    def act_upon_pressed_keys(self) -> None:
        keys = pygame.key.get_pressed()

        if keys[pygame.K_a]:
            self.mode = "AUTOMÁTICO"
            self.last_command = "Ativou modo automático"
            self.velocity_x = 2.0
        elif keys[pygame.K_m]:
            self.mode = "MANUAL"
            self.last_command = "Ativou modo manual"
            self.velocity_x = 0.0

        if self.mode == "MANUAL":
            if keys[pygame.K_LEFT]:
                self.velocity_x = -4.0
                self.last_command = "Mandou o robô ir para a esquerda"
            elif keys[pygame.K_RIGHT]:
                self.velocity_x = 4.0
                self.last_command = "Mandou o robô ir para a direita"
            elif keys[pygame.K_s]:
                self.velocity_x = 0.0
                self.last_command = "Mandou o robô parar"

    def update_state(self) -> None:
        if self.mode == "AUTOMÁTICO":
            self.velocity_x = 2.0

        self.position_x += self.velocity_x

        # Talvez não seja ideal colocar a posição x, já que não tem limite
        self.position_x = max(20, min(self.position_x, self.screen_width - 120))

        self.lidar_distance = 90 #+ 40 * math.sin(self.position_x / 40.0)

        self.position_history.append(self.position_x)
        self.velocity_history.append(self.velocity_x)
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

        self.draw_text("Estado do robô", 820, 20, (255, 255, 255))
        self.draw_text(f"Modo: {self.mode}", 820, 60)
        self.draw_text(f"Posição: {self.position_x:.1f}", 820, 90)
        self.draw_text(f"Velocidade: {self.velocity_x:.1f}", 820, 120)
        self.draw_text(f"LIDAR: {self.lidar_distance:.1f} m", 820, 150)
        self.draw_text(f"Último comando:", 820, 185)
        self.draw_text(self.last_command, 820, 210, (200, 200, 120))

        self.draw_text("Comandos disponíveis", 820, 260)
        self.draw_text("A -> Ativar modo automático", 820, 290)
        self.draw_text("M -> Ativar modo manual", 820, 320)

        if self.mode == "AUTOMÁTICO":
            self.draw_text("Opções abaixo desabilitadas.", 820, 360, (220, 140, 140))
        else:
            self.draw_text("Opções abaixo habilitadas.", 820, 360, (140, 220, 140))

        self.draw_text("← -> Esquerda (manual)", 820, 400)
        self.draw_text("→ -> Direita (manual)", 820, 430)
        self.draw_text("S -> Parar (manual)", 820, 460)

        position_rect = pygame.Rect(20, 20, 760, 200)
        velocity_rect = pygame.Rect(20, 240, 760, 200)
        lidar_rect = pygame.Rect(20, 460, 760, 200)

        self.draw_graph(self.position_history, position_rect, (80, 220, 120), "Posição (m)", 0, self.screen_width)
        self.draw_graph(self.velocity_history, velocity_rect, (240, 180, 40), "Velocidade (m/frame)", -6, 6)
        self.draw_graph(self.lidar_history, lidar_rect, (180, 100, 240), "Distância LIDAR (m)", 20, 140)

    def run(self) -> None:
        while self.running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False

            self.act_upon_pressed_keys()
            self.update_state()
            self.draw_interface()

            pygame.display.flip()
            self.clock.tick(self.FPS)

        pygame.quit()


if __name__ == "__main__":
    interface = RemoteOperationInterface()
    interface.run()
