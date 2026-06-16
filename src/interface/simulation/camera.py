import pygame
import robot
import tunnel

class Camera():
    def __init__(self, tunnel : tunnel, robot : robot, screen: pygame.display) -> None:
        self.bounded_character = robot
        self.bounded_tunnel = tunnel
        self.bounded_screen = screen
        self.off_set_x = 0
        self.off_set_y = 0 
        self.camera_is_moving = False
        screen_w, screen_h = self.bounded_screen.get_size()
        self.screen_w = screen_w
        self.screen_h = screen_h
        self.speed = max(1, int(screen_w * 0.00625))

    def off_set_map(self, off_set_x : int, off_set_y : int) -> None:
        """Movimenta o mapa em relação ao robot
        """
        self.off_set_x += off_set_x
        self.bounded_tunnel.off_set_x = self.off_set_x
        self.off_set_y += off_set_y
        self.bounded_tunnel.off_set_y = self.off_set_y
        self.bounded_tunnel.render_visible_layers(self.bounded_screen, self.off_set_x, self.off_set_y)

    def follow_robot(self) -> None:
        """ Garante o posicionamento da câmera próximo ao robot, movendo a câmera sempre que o robot permanecer em posições específicas
        """
        screen_w, screen_h = self.bounded_screen.get_size()
        # thresholds relative to a base 1280x720 reference
        left_threshold = int(screen_w * (400.0 / 1280.0))
        right_threshold = int(screen_w * (720.0 / 1280.0))
        top_bound = int(screen_h * (-212.5 / 720.0))
        bottom_bound = int(screen_h * (320.0 / 720.0))

        if (self.off_set_x > -4 * (screen_w / 1280.0)):
            self.bounded_character.x_limit_reached = True
        else:
            self.bounded_character.x_limit_reached = False

        if (self.off_set_y < top_bound) or (self.off_set_y > bottom_bound):
            self.bounded_character.y_limit_reached = True
        else:
            self.bounded_character.y_limit_reached = False

        char_x = self.bounded_character.rect.topleft[0]
        if (char_x <= left_threshold and self.bounded_character.speed[0] < 0) and not (self.off_set_x > -4 * (screen_w / 1280.0)):
            self.off_set_map(self.speed, 0)
        elif (char_x >= right_threshold and self.bounded_character.speed[0] > 0):
            self.off_set_map(-self.speed, 0)

        # if (self.bounded_character.rect.topleft[1] <= 200 and self.bounded_character.vertical_speed > 0) and not(self.off_set_y < -212.5):
        #     self.off_set_map(0, -self.bounded_character.delta_pos_y)
        # elif self.bounded_character.rect.topleft[1] >= 350 and not self.bounded_character.is_colliding(self.bounded_tunnel) and not(self.off_set_y > 320):
        #     self.off_set_map(0, -self.bounded_character.delta_pos_y)
        else:
            self.off_set_map(0, 0)