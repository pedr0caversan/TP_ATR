import pygame
import time
from pathlib import Path

from MQTTInterface import MQTTInterface
from tunnel import Tunnel
from robot import Robot
from camera import Camera

_BASE_DIR = Path(__file__).parent


class Simulation:
    def __init__(self) -> None:
        self.clock = pygame.time.Clock()

        pygame.font.init()

        # Game Screen
        screen_width = 1280
        screen_height = 720
        self.screen = pygame.display.set_mode((screen_width, screen_height))
        pygame.display.set_caption("Simulação")
        self.running = True

        self.tunnel = Tunnel(str(_BASE_DIR / "tunnel"))
        self.robot = Robot()
        self.robot_group = pygame.sprite.Group()
        self.robot_group.add(self.robot)
        self.my_camera = Camera(self.tunnel, self.robot, self.screen)
        self.FPS = 60

        # MQTT
        self.mqtt = MQTTInterface(actuator_callback=self.update_control_effort)
        self.mqtt.connect()
        self.control_effort = 0.0

    def update_control_effort(self, data: float) -> None:
        self.control_effort = data

    def update_sensor_data(self) -> None:
        self.robot.update_encoder()
        self.robot.update_lidar(self.tunnel, self.my_camera.off_set_x)

    def act_upon_pressed_keys(self) -> None:
        """Toma as ações necessárias a respeito das teclas pressionadas no teclado."""
        keys = pygame.key.get_pressed()

        if keys[pygame.K_LEFT]:
            if not keys[pygame.K_RIGHT] and not self.robot.is_left_colliding(
                self.tunnel
            ):
                self.robot.update_position(-self.robot.max_horizontal_speed, 0)

        if keys[pygame.K_RIGHT]:
            if not keys[pygame.K_LEFT]:
                self.robot.update_position(self.robot.max_horizontal_speed, 0)

        if not keys[pygame.K_LEFT] and not keys[pygame.K_RIGHT]:
            self.robot.update_position(0, 0)

    def control_robot(self) -> None:
        """Atualiza as animações do robo e desenha elas na tela"""
        self.my_camera.follow_robot()

        # atualiza todas animações
        self.robot.draw_collision_rect(self.screen)
        self.robot.animate()

        self.robot_group.draw(self.screen)
        self.robot_group.update()

    def move_robot(self) -> None:
        """Movimenta o robô de acordo com a física da simulação, levando em consideração gravidade e dinâmica horizontal de movimento"""
        if not self.robot.is_colliding(self.tunnel):
            self.robot.apply_delta_gravity_effect(0.003, self.tunnel)
        else:
            self.robot.correct_ground_intersection(self.tunnel)
        if self.robot.is_colliding(self.tunnel):
            self.robot.vertical_speed = 0
        self.robot.apply_horizontal_velocity_effect(self.control_effort, 1 / self.FPS)

    def simulation_run(self):

        while self.running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False

            current_time = time.time()

            self.move_robot()
            
            self.control_robot()

            # TODO: Apagar duas linhas abaixo na versão final, não haverá controle por teclado na simulação
            self.act_upon_pressed_keys()

            self.control_robot()

            self.update_sensor_data()

            # print(
            #     f"Lidar: {self.robot.lidar:.2f} m, Encoder: {self.robot.encoder:.2f} m"
            # )
            self.mqtt.publish_sensor_data(self.robot.lidar, self.robot.encoder)

            pygame.display.flip()

            self.clock.tick(self.FPS)

        pygame.quit()


# Apenas existe para ser possivel rodar a Game Sem utilizar o menu
if __name__ == "__main__":
    my_game = Simulation()
    my_game.simulation_run()
