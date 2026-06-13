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

        self._load_base_images()

        self.tile_image_map = {} # mapeia tile_number -> index da imagem em self.image_layers
        self.last_chosen_index = None
        self.scaled_cache = {} # cache: (w,h) -> [scaled_surfaces...]

        ground = pygame.Rect(0, 670, 1280, 50)
        entrance = pygame.Rect(-10, 0, 10, 720)
        ceiling = pygame.Rect(0, 0, 1280, 80)

        self.colision_rects = [ground, entrance, ceiling]

    def _load_base_images(self) -> None:
        valid_extensions = {'.png', '.jpg', '.jpeg', '.bmp', '.gif', '.webp'}
        path = Path(self.map_folder)

        if path.is_file() and path.suffix.lower() in valid_extensions:
            image_paths = [path]
        elif path.is_dir():
            image_paths = sorted(
                [file for file in path.iterdir() if file.is_file() and file.suffix.lower() in valid_extensions]
            )
        else:
            raise FileNotFoundError(f"Pasta ou arquivo de imagem inválido: {self.map_folder}")

        if not image_paths:
            raise FileNotFoundError(f"Nenhuma imagem de túnel encontrada em '{self.map_folder}'")

        self.image_layers = image_paths[:3]
        if len(self.image_layers) < 3:
            self.image_layers = (self.image_layers * 3)[:3]

        self.image_layers = [pygame.image.load(str(img)).convert_alpha() for img in self.image_layers]

    def render_visible_layers(self, screen: pygame.Surface, off_set_x: float, off_set_y: float) -> None:
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
                pygame.transform.scale(img, size_key) if img.get_size() != size_key else img
                for img in self.image_layers
            ]
        scaled_images = self.scaled_cache[size_key]

        # offset modular para posicionar duas tiles contínuas
        offset = scroll_x % screen_width
        left_x = offset - screen_width
        right_x = offset

        # número das tiles (inteiros) usados para escolher imagens consistentemente
        tile_number_right = math.floor(scroll_x / screen_width)
        tile_number_left = tile_number_right - 1

        def choose_image_for(tile_number: int) -> int:
            if tile_number in self.tile_image_map:
                return self.tile_image_map[tile_number]
            choices = list(range(len(scaled_images)))
            if self.last_chosen_index in choices and len(choices) > 1:
                choices.remove(self.last_chosen_index)
            choice = random.choice(choices)
            self.tile_image_map[tile_number] = choice
            self.last_chosen_index = choice
            # limpeza simples: manter só janelas recentes (ex.: 10 tiles)
            keys = sorted(self.tile_image_map.keys())
            while len(keys) > 10:
                del self.tile_image_map[keys.pop(0)]
            return choice

        left_img = scaled_images[choose_image_for(tile_number_left)]
        right_img = scaled_images[choose_image_for(tile_number_right)]

        screen.blit(left_img, (left_x + self.x_correction, scroll_y))
        screen.blit(right_img, (right_x + self.x_correction, scroll_y))

        for obj in self.colision_rects:
            pygame.draw.rect(screen, (255, 0, 0), obj, 2)

    def check_collision(self, robot_rect: pygame.rect) -> bool:
        """Detecta se o robot está colidindo com algum rect do mapa
        """
        for object in self.colision_rects:
            if robot_rect.colliderect(object):
                return True
        return False
    
    def return_ground_intersection(self, robot_rect: pygame.rect) -> pygame.rect:
        """Retorna o rect de interseção entre o rect de pé do player e algum objeto do mapa 
        """
        for object in self.colision_rects:
            if robot_rect.colliderect(object):
                    intersection = object.clip(robot_rect)
                    return intersection
 