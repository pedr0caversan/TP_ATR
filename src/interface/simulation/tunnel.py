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

        self.colision_rects = [pygame.Rect(0, 500, 1000, 200)]

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
        x_position = self.x_correction + self.off_set_x - off_set_x
        y_position = self.y_correction + self.off_set_y - off_set_y

        for image in self.image_layers:
            if image.get_size() != (screen_width, screen_height):
                rendered_image = pygame.transform.scale(image, (screen_width, screen_height))
            else:
                rendered_image = image

            screen.blit(rendered_image, (x_position, y_position))

        for object in self.colision_rects:
            scaled_x = object.x * self.scale_factor + self.x_correction
            scaled_y = object.y * self.scale_factor - self.y_correction
            scaled_width = object.width * self.scale_factor
            scaled_height = object.height * self.scale_factor
            rect = pygame.Rect(scaled_x + self.off_set_x, scaled_y - self.off_set_y, scaled_width, scaled_height)
            # Desenha um retângulo vermelho para representar a área de colisão, com uma borda de 1 pixel
            pygame.draw.rect(screen, (255, 0, 0), rect, 2) 

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
 