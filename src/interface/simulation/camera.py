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

    def off_set_map(self, off_set_x : int, off_set_y : int) -> None:
        """Movimenta o mapa em relação ao robot
        """
        self.off_set_x += off_set_x
        self.bounded_tunnel.off_set_x = self.off_set_x
        self.off_set_y += off_set_y
        self.bounded_tunnel.off_set_y = self.off_set_y
        self.bounded_tunnel.render_visible_layers(self.bounded_screen, self.off_set_x, self.off_set_y)
        self.speed = 8

    def follow_robot(self) -> None:
        """ Garante o posicionamento da câmera próximo ao robot, movendo a câmera sempre que o robot permanecer em posições específicas
        """
        self.bounded_character.walked_distance(self.off_set_x)
        self.bounded_character.ceiling_distance(self.bounded_tunnel, self.off_set_x)

        if (self.off_set_x > -4):
            self.bounded_character.x_limit_reached = True
        else:
            self.bounded_character.x_limit_reached = False
     
        if (self.off_set_y < -212.5) or (self.off_set_y > 320):
            self.bounded_character.y_limit_reached = True
        else:
            self.bounded_character.y_limit_reached = False
   
        if (self.bounded_character.rect.topleft[0] <= 400 and self.bounded_character.speed[0] < 0) and not(self.off_set_x > -4):
            self.off_set_map(self.speed, 0)
        elif (self.bounded_character.rect.topleft[0] >= 720 and self.bounded_character.speed[0] > 0):
            self.off_set_map(-self.speed, 0)

        # if (self.bounded_character.rect.topleft[1] <= 200 and self.bounded_character.vertical_speed > 0) and not(self.off_set_y < -212.5):
        #     self.off_set_map(0, -self.bounded_character.delta_pos_y)
        # elif self.bounded_character.rect.topleft[1] >= 350 and not self.bounded_character.is_colliding(self.bounded_tunnel) and not(self.off_set_y > 320):
        #     self.off_set_map(0, -self.bounded_character.delta_pos_y)
        else:
            self.off_set_map(0, 0)