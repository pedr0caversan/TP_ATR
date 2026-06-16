import math
import pygame
import tunnel
from pathlib import Path

_BASE_DIR = Path(__file__).parent

PRINT_EVERY_N = 15
class Robot(pygame.sprite.Sprite):
    def __init__(self) -> None:
        super().__init__()
        # Sprite Vectors
        self.__sprites_idle = []

        # Load All Sprites
        self.__import_sprites(1, _BASE_DIR / "robot/robo.png", self.__sprites_idle)

        # Default Boolean and Character States
        self.is_animating = False
        self.direction = "right"
        self.current_sprite = 0
        self.image = self.__sprites_idle[self.current_sprite]

        # Default Position and movement
        self.gravity_ = 25000
        self.vertical_speed = 0
        self.horizontal_speed = 0
        self.pos_x = 10
        self.pos_y = 400
        self.width = 100 * 2  # Largura do sprite do robo
        self.height = 80 * 2  # Altura do sprite do robo
        self.rect_down = pygame.Rect(
            self.pos_x + 20, self.pos_y + 100, self.width - 40, self.height - 100
        )
        self.rect_left = pygame.Rect(self.pos_x, self.pos_y + 50, 10, self.height - 100)
        self.rect = pygame.Rect(self.pos_x, self.pos_y, self.width, self.height)
        self.rect.topleft = [self.pos_x, self.pos_y]
        self.speed = [0.0, 0.0]
        self.max_horizontal_speed = 8
        self.delta_pos_y = 0
        self.x_limit_reached = False
        self.y_limit_reached = False

        self.pixels_per_meter = 72

        self.encoder_dist = 0
        self.encoder = False
        self.lidar = 0
        self._encoder_print_counter = 0
        
        self.debug_counter = 0

    # TODO: TERMINAR O DOCSTRING
    def __import_sprites(
        self, number_of_sprites: int, arquive: str, sprites_vector: list
    ) -> None:
        """Acessa a pasta selecionada {arquive} e guarda os PNG em um vetores de PNG {sprites_vector}.

        Args:
            number_of_sprites: número de sprites da animação específica do sprites_vector enviado como parâmetro
            arquive: nome do arquivo relacionado ao grupo de sprites que será importado para o sprites_vector
            sprites_vector: vetor de sprites para onde serão importados os sprites

        """
        scale = 2
        for i in range(number_of_sprites):
            sprite = pygame.image.load(str(arquive)).convert_alpha()
            # Scale the sprite
            sprite = pygame.transform.scale(
                sprite,
                (int(sprite.get_width() * scale), int(sprite.get_height() * scale)),
            )
            sprites_vector.append(sprite)
        print(
            f"Sprites importados para {sprites_vector}: {len(sprites_vector)} sprites carregados."
        )

    def update_position(self, new_pos_x: int, new_pos_y: int) -> None:
        """Muda a posição do Rect do robô e a posição dos seu rects direcionais

        Args:
            new_pos_x: valor a ser somado à posição da direção horizontal
            new_pos_y: o mesmo, porém à direção vertical
        """
        self.speed[0] = new_pos_x
        self.speed[1] = new_pos_y
        if new_pos_x < 0:
            self.direction = "left"
        if new_pos_x > 0:
            self.direction = "right"
        if not (
            ((self.rect.topleft[0] <= 400) and self.direction == "left")
            or ((self.rect.topleft[0] >= 720) and self.direction == "right")
        ) or (self.x_limit_reached):
            self.pos_x += self.speed[0]
        if not self.y_limit_reached:
            self.pos_y += self.speed[1]
        self.rect.topleft = [self.pos_x, self.pos_y]
        self.rect_down.topleft = [self.pos_x + 20, self.pos_y + 100]
        self.rect_left.topleft = [self.pos_x, self.pos_y + 50]

    def apply_delta_gravity_effect(self, delta_t: float, tunnel: tunnel) -> None:
        """Modifica a posição vertical do jogador de acordo com as leis da gravidade no tempo delta_t.

        Args:
            delta_t: tempo que determina o delta posição
        """
        self.delta_pos_y = (
            self.vertical_speed * delta_t - self.gravity_ * delta_t * delta_t / 2
        )
        # delta(X) = Vot - g(t^2)/2
        self.vertical_speed -= self.gravity_ * delta_t
        # V = Vo - gt
        self.update_position(0, -self.delta_pos_y)

    def apply_horizontal_velocity_effect(
        self, control_effort: float, delta_t: float
    ) -> None:
        """Atualiza a velocidade horizontal com modelo de 1ª ordem (motor + atrito de rolamento)
        e em seguida atualiza a posição.

        Dinâmica: m*dv/dt = F_motor - F_atrito
                           = u*a_max - k_atrito*v

        Args:
            control_effort: esforço do atuador em percentual [-100, 100]
            delta_t: intervalo de tempo em segundos
        """
        u = control_effort / 100.0  # normaliza para [-1, 1]

        # Parâmetros físicos
        # a_max: aceleração com atuador saturado (pixels/s²)
        # TODO (Pedro, Davi): Decidir quantos pixels correspondem a 1 metro na simulação
        # TODO (Pedro): tunar os parâmetros pro movimento do robô ficar natural
        # k_atrito: coeficiente de atrito de rolamento (1/s), derivado de a_max e v_max
        # obs: Tau = v_max/a_max = 1/k_atrito
        a_max = 24.0
        k_atrito = (
            a_max / self.max_horizontal_speed
        )  # garante v_ss = max_horizontal_speed

        # motor menos atrito
        accel = u * a_max - k_atrito * self.horizontal_speed
        self.horizontal_speed += accel * delta_t

        # Atualiza posição com a velocidade calculada
        delta_x = self.horizontal_speed * delta_t
        self.update_position(delta_x, 0)

    def correct_ground_intersection(self, tunnel: tunnel) -> None:
        """Coloca o player precisamente acima do chão após uma queda.

        Args:
            tunnel: objeto capaz de retornar a interseção entre o rect inferior do player e o rect do chão.
        """
        intersection_rect = tunnel.return_ground_intersection(self.rect_down)
        if intersection_rect.height > 1:
            self.update_position(0, -intersection_rect.height + 1)
            print(f"Interseção corrigida: {intersection_rect.height} pixels ajustados.")

    def update_encoder(self, offset_camera: int):
        traveled_distance = self.pos_x - offset_camera
        print("MÓDULO: %.2f metros percorridos" % (traveled_distance -self.encoder_dist))
        if math.fabs(traveled_distance - self.encoder_dist) >= self.meter:
            self.encoder_dist = traveled_distance
            self.encoder = 1 if self.encoder == 0 else 0
            self._encoder_print_counter += 1
            if self._encoder_print_counter >= 15:
                self._encoder_print_counter = 0
                print(
                    f"Encoder atualizado: {self.encoder} (distância total: {self._total_distance / self.pixels_per_meter:.2f} m)"
                )

    # TODO: IMPLEMENTAR RUÍDO DE MEDIÇÃO
    def update_lidar(self, tunnel: tunnel, offset_camera: int):
        distance = tunnel.return_distance_to_ceiling(self.rect, offset_camera)
        if distance is not None:
            self.lidar = distance / self.pixels_per_meter
            self.debug_counter += 1
            if self.debug_counter >= PRINT_EVERY_N:
                self.debug_counter = 0
                print("Lidar: {:.2f} m".format(self.lidar))

    def is_colliding(self, tunnel: tunnel) -> bool:
        """Retorna True se o robo estiver colidindo com o solo

        Args:
            tunnel: objeto que contém função capaz de checar colisões
        """
        return tunnel.check_collision(self.rect_down)

    def is_left_colliding(self, tunnel: tunnel) -> bool:
        """Retorna True se o robo estiver colidindo pelo lado esquerdo

        Args:
            tunnel: objeto que contém função capaz de checar colisões
        """
        return tunnel.check_collision(self.rect_left)

    def animate(self) -> None:
        self.is_animating = True

    # TODO: FAZER A DOCSTRING
    def update(self) -> None:
        """Atualiza"""
        self.image = self.__sprites_idle[int(self.current_sprite)]

    def draw_collision_rect(self, screen: pygame.display) -> None:
        """Desenha os rects do player na tela.

        Função com uso estrito para testes relacionados aos rects do player.
        """
        # Desenha um retângulo vermelho em torno do retângulo do jogador
        green = (0, 255, 0)
        red = (255, 0, 0)
        white = (255, 255, 255)
        black = (0, 0, 0)
        pygame.draw.rect(screen, green, self.rect, 1)
        pygame.draw.rect(screen, black, self.rect_down, 1)
        pygame.draw.rect(screen, white, self.rect_left, 1)

        altura = self.lidar * self.pixels_per_meter
        pygame.draw.line(
            screen,
            (255, 255, 255),
            (self.pos_x + (self.width / 2), self.pos_y - altura),
            (self.pos_x + (self.width / 2), self.pos_y),
            1,
        )
