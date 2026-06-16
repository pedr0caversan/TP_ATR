import math
import random
import pygame
from pathlib import Path


class Tunnel:
    def __init__(self, map_folder: str) -> None:
        self.scale_factor = 1
        self.x_correction = 0
        self.y_correction = 0
        self.off_set_x = 0
        self.off_set_y = 0
        self.map_folder = map_folder
        self.image_layers = []

        self.colision_by_image = {}
        self.ground = pygame.Rect(0, 670, 1280, 50)
        self.entrance = pygame.Rect(-10, 0, 10, 720)

        self._load_base_images()

        self.tile_image_map = {}  # mapeia tile_number -> index da imagem em self.image_layers
        self.last_chosen_index = None
        self.scaled_cache = {}  # cache: (w,h) -> [scaled_surfaces...]
        self.old_off_set_x = 0

        self.left_tile = 0
        self.right_tile = 0
        # DEBUG
        self._debug_conter = 0

    def _load_base_images(self) -> None:
        tunnel_files = ["tunel.png", "tunel_buraco.png", "tunel_relevo.png"]

        path = Path(self.map_folder)

        image_paths = []

        for file in tunnel_files:
            file_path = path.joinpath(file)

            if file_path.is_file():
                image_paths.append(file_path)
            else:
                print(f"{file_path} not found")

            if file == "tunel.png":
                self.colision_by_image[0] = []
            elif file == "tunel_buraco.png":
                self.colision_by_image[1] = []
            elif file == "tunel_relevo.png":
                self.colision_by_image[2] = []

        self.image_layers = [
            pygame.image.load(str(img)).convert_alpha() for img in image_paths
        ]

    def associate_image_to_object(self, tile: int, offset_x: int):
        if tile == 0:
            ceiling = pygame.Rect(offset_x, 0, 1280, 80)
            self.colision_by_image[0] = [self.entrance, self.ground, ceiling]
        elif tile == 1:
            ceiling1 = pygame.Rect(offset_x, 0, 550, 80)
            ceiling2 = pygame.Rect(offset_x + 700, 0, 580, 80)
            hole = pygame.Rect(offset_x + 550, 0, 150, 1)
            self.colision_by_image[1] = [
                self.entrance,
                self.ground,
                ceiling1,
                ceiling2,
                hole,
            ]
        elif tile == 2:
            ceiling1 = pygame.Rect(offset_x, 0, 600, 80)
            ceiling2 = pygame.Rect(offset_x + 650, 0, 630, 80)
            obstacle = pygame.Rect(offset_x + 600, 0, 50, 160)
            self.colision_by_image[2] = [
                self.entrance,
                self.ground,
                ceiling1,
                ceiling2,
                obstacle,
            ]

    def render_visible_layers(
        self, screen: pygame.Surface, off_set_x: float, off_set_y: float
    ) -> None:
        """Renderiza o túnel a partir das imagens base carregadas.

        Args:
            screen: display no qual o jogo será aberto
            off_set_x: deslocamento horizontal aplicado na renderização
            off_set_y: deslocamento vertical aplicado na renderização
        """
        screen_width, screen_height = screen.get_size()
        scroll_x = off_set_x + self.x_correction
        scroll_y = off_set_y + self.y_correction

        # escala e cache das imagens conforme o tamanho da tela
        size_key = (screen_width, screen_height)
        if size_key not in self.scaled_cache:
            self.scaled_cache[size_key] = [
                pygame.transform.scale(img, size_key)
                if img.get_size() != size_key
                else img
                for img in self.image_layers
            ]
        scaled_images = self.scaled_cache[size_key]

        # offset modular para posicionar duas tiles contínuas
        offset = scroll_x % screen_width

        # Importante porque o left_x e o right_x devem decrescer, porém a operação módulo funciona ao contrário com números negativos, então PRECISO setar offset caso ele seja 0
        if offset == 0:
            offset = 1280

        left_x = offset - screen_width
        right_x = offset

        # número das tiles (inteiros) usados para escolher imagens consistentemente
        tile_number_right = math.floor(-scroll_x / screen_width)
        tile_number_left = tile_number_right - 1

        def choose_image_for(tile_number: int) -> int:
            restarting = False

            if tile_number in self.tile_image_map:
                self.last_chosen_index = self.tile_image_map[tile_number]
                return self.tile_image_map[tile_number]

            choices = list(range(len(scaled_images)))
            if self.last_chosen_index in choices and len(choices) > 1:
                choices.remove(self.last_chosen_index)
            choice = random.choice(choices)

            chaves = (
                sorted(self.tile_image_map.keys())
                if len(self.tile_image_map) > 0
                else [-1]
            )
            if tile_number < chaves[0]:
                restarting = True

            self.tile_image_map[tile_number] = choice
            self.last_chosen_index = choice

            # limpeza simples: manter só janelas recentes (ex.: 10 tiles)
            keys = sorted(self.tile_image_map.keys())

            while len(keys) > 10:
                if restarting:
                    del self.tile_image_map[keys.pop()]
                else:
                    del self.tile_image_map[keys.pop(0)]
            return choice

        # Se carro estiver movendo para direita, atualiza left tile primeiro
        if self.old_off_set_x - off_set_x >= 0:
            self.old_off_set_x = off_set_x
            left_tile_selected = choose_image_for(tile_number_left)
            right_tile_selected = choose_image_for(tile_number_right)
        # Se carro estiver movendo para esquerda, atualiza right tile primeiro
        elif self.old_off_set_x - off_set_x < 0:
            self.old_off_set_x = off_set_x
            right_tile_selected = choose_image_for(tile_number_right)
            left_tile_selected = choose_image_for(tile_number_left)

        # print(f"Mapa de tiles: {self.tile_image_map}, Last choice: {self.last_chosen_index}, Left tile: {tile_number_left}")
        self.left_tile = left_tile_selected
        self.right_tile = right_tile_selected

        left_img = scaled_images[left_tile_selected]
        right_img = scaled_images[right_tile_selected]

        screen.blit(left_img, (left_x + self.x_correction, scroll_y))
        screen.blit(right_img, (right_x + self.x_correction, scroll_y))

        self.associate_image_to_object(left_tile_selected, left_x + self.x_correction)
        self.associate_image_to_object(right_tile_selected, right_x + self.x_correction)

        for obj in self.colision_by_image[left_tile_selected]:
            pygame.draw.rect(screen, (255, 0, 0), obj, 2)
        for obj in self.colision_by_image[right_tile_selected]:
            pygame.draw.rect(screen, (255, 0, 0), obj, 2)

    def check_collision(self, robot_rect: pygame.rect) -> bool:
        """Detecta se o robot está colidindo com algum rect do mapa"""
        for object in self.colision_by_image[self.left_tile]:
            if robot_rect.colliderect(object):
                return True
        for object in self.colision_by_image[self.right_tile]:
            if robot_rect.colliderect(object):
                return True
        return False

    def return_distance_to_ceiling(self, robot_rect: pygame.rect, offset_x: int) -> int:
        """Retorna a distância entre o topo do robô e o objeto do mapa que esteja acima dele.

        Args:
            robot_rect: retângulo do robô em coordenadas de câmera
            offset_x: não é mais necessário, mantido por compatibilidade

        Returns:
            Distância até o topo do objeto mais próximo acima do robô, ou None se nenhum for encontrado
        """
        for object in self.colision_by_image[self.left_tile]:
            # Verifica se o robô está horizontalmente alinhado com o objeto
            if (
                object.x
                <= (robot_rect.x + (robot_rect.width / 2))
                <= object.x + object.width
            ) and (object.y < robot_rect.y):
                # Distância do topo do robô até o topo do objeto
                distance = robot_rect.y - (object.y + object.height)
                # self._debug_conter += 1
                # if self._debug_conter >= 60:
                #     self._debug_conter = 0
                #     print(f"Distância para teto (esquerda): {distance} pixels")
                return distance

        for object in self.colision_by_image[self.right_tile]:
            # Verifica se o robô está horizontalmente alinhado com o objeto
            if (
                object.x
                <= (robot_rect.x + (robot_rect.width / 2))
                <= object.x + object.width
            ) and (object.y < robot_rect.y):
                # Distância do topo do robô até o topo do objeto
                distance = robot_rect.y - (object.y + object.height)
                return distance

    def return_ground_intersection(self, robot_rect: pygame.rect) -> pygame.rect:
        """Retorna o rect de interseção entre o rect de pé do player e algum objeto do mapa"""
        for object in self.colision_by_image[self.left_tile]:
            if robot_rect.colliderect(object):
                intersection = object.clip(robot_rect)
                return intersection
        for object in self.colision_by_image[self.right_tile]:
            if robot_rect.colliderect(object):
                intersection = object.clip(robot_rect)
                return intersection
