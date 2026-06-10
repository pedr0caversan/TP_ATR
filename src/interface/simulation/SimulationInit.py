import pygame
import time

from tunnel import Tunnel

class Simulation():
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

    def simulation_run(self):

        while self.running:

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False

            current_time = time.time()

            self.tunnel.render_visible_layers(self.screen, 0, 0)

            pygame.display.flip()

            self.clock.tick(60)

        pygame.quit()


# Apenas existe para ser possivel rodar a Game Sem utilizar o menu
if __name__ ==  '__main__':
    my_game = Simulation()
    my_game.simulation_run()

        