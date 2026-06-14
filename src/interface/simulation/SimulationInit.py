import pygame
import time

from tunnel import Tunnel
from robot import Robot
from camera import Camera


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

        self.tunnel = Tunnel("./assets/")
        self.robot = Robot()
        self.robot_group = pygame.sprite.Group()
        self.robot_group.add(self.robot)
        self.my_camera = Camera(self.tunnel, self.robot, self.screen)

    def act_upon_pressed_keys(self) -> None:
        """Toma as ações necessárias a respeito das teclas pressionadas no teclado.

        Permite a movimentação lateral, o pulo, o ataque e o heal. As ações necessárias - como mudanças de variável - para regular
        as animações e atributos do player e inimigos são tomadas em cada bloco condicional de acordo com a necessidade.

        Args:
            current_time: tempo atual fornecido pela biblioteca time
        """
        keys = pygame.key.get_pressed()

        if keys[pygame.K_LEFT]:
            if not keys[pygame.K_RIGHT] and not self.robot.is_left_colliding(
                self.tunnel
            ):
                self.robot.update_position(-self.robot.max_horizontal_speed, 0)

        if keys[pygame.K_RIGHT]:
            if not keys[pygame.K_LEFT]:
                self.robot.update_position(self.robot.max_horizontal_speed, 0)

    def control_robot(self) -> None:
        """Atualiza as animações do robo e desenha elas na tela"""
        self.my_camera.follow_robot()

        # atualiza todas animações
        self.robot.draw_collision_rect(self.screen)
        self.robot.animate()

        self.robot_group.draw(self.screen)
        self.robot_group.update()

    def apply_gravity_to_robot(self) -> None:
        """Aplica gravidade ao robot e garante que ele caia no chão corretamente"""
        if not self.robot.is_colliding(self.tunnel):
            self.robot.apply_delta_gravity_effect(0.003, self.tunnel)
            # TODO (Pedro): periodizar tempo da task para d_t fazer sentido e pegar esforço de controle por MQTT pra passar para a função abaixo
            # self.robot.apply_horizontal_velocity_effect()
        else:
            self.robot.correct_ground_intersection(self.tunnel)
        if self.robot.is_colliding(self.tunnel):
            self.robot.vertical_speed = 0

    def simulation_run(self):

        while self.running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False

            current_time = time.time()

            self.apply_gravity_to_robot()

            self.act_upon_pressed_keys()

            self.control_robot()

            self.robot_group.draw(self.screen)
            self.robot_group.update()

            pygame.display.flip()

            self.clock.tick(60)

        pygame.quit()


# Apenas existe para ser possivel rodar a Game Sem utilizar o menu
if __name__ == "__main__":
    my_game = Simulation()
    my_game.simulation_run()
