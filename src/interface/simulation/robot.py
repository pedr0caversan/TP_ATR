import pygame
import tunnel

class Robot(pygame.sprite.Sprite):
    def __init__(self) -> None:
        super().__init__()
        # Sprite Vectors
        self.__sprites_idle = []

        # Load All Sprites
        self.__import_sprites(1,'assets/robo.jpg', self.__sprites_idle)

        # Default Boolean and Character States
        self.is_animating = False
        self.current_sprite = 0
        self.image = self.__sprites_idle[self.current_sprite]     

        # Default Position and movement
        self.gravity_ = 25000
        self.vertical_speed = 0
        self.pos_x = 0
        self.pos_y = 0
        self.width = 500 / 2# Largura do sprite do robo
        self.height = 240 / 2# Altura do sprite do robo
        self.rect_down = pygame.Rect(self.pos_x, self.pos_y+100, self.width, self.height-100)
        self.rect = pygame.Rect(self.pos_x, self.pos_y, self.width, self.height)
        self.rect.topleft = [self.pos_x, self.pos_y]
        self.speed = [0.0, 0.0]
        self.max_horizontal_speed = 8
        self.delta_pos_y = 0
        self.x_limit_reached = False
        self.y_limit_reached = False
        
    #TODO: TERMINAR O DOCSTRING
    def __import_sprites(self, number_of_sprites: int, arquive: str, sprites_vector: list) -> None:

        """ Acessa a pasta selecionada {arquive} e guarda os PNG em um vetores de PNG {sprites_vector}.

        Args:
            number_of_sprites: número de sprites da animação específica do sprites_vector enviado como parâmetro
            arquive: nome do arquivo relacionado ao grupo de sprites que será importado para o sprites_vector
            sprites_vector: vetor de sprites para onde serão importados os sprites
    
        """
        scale = 0.5
        for i in range(number_of_sprites):
            sprite = pygame.image.load(str(arquive)).convert_alpha()
            # Scale the sprite
            sprite = pygame.transform.scale(sprite, (int(sprite.get_width() * scale), int(sprite.get_height() * scale)))
            sprites_vector.append(sprite)
        print(f"Sprites importados para {sprites_vector}: {len(sprites_vector)} sprites carregados.")

    def update_position(self, new_pos_x: int, new_pos_y: int) -> None:
        """ Muda a posição do Rect do player e a posição dos seu rects direcionais

        Args:
            new_pos_x: valor a ser somado à posição da direção horizontal
            new_pos_y: o mesmo, porém à direção vertical
        """
        self.speed[0] = new_pos_x
        self.speed[1] = new_pos_y
        self.pos_x += self.speed[0]
        self.pos_y += self.speed[1]
        self.rect.topleft = [self.pos_x, self.pos_y]
        self.rect_down.topleft = [self.pos_x, self.pos_y+100]    

    def apply_delta_gravity_effect(self, delta_t: float, tunnel: tunnel) -> None:
        """Modifica a posição vertical do jogador de acordo com as leis da gravidade no tempo delta_t.

        Args:
            delta_t: tempo que determina o delta posição 
        """
        self.delta_pos_y = self.vertical_speed*delta_t - self.gravity_*delta_t*delta_t/2 
        #delta(X) = Vot - g(t^2)/2
        self.vertical_speed -= self.gravity_*delta_t
         #V = Vo - gt
        self.update_position(0, -self.delta_pos_y)

    def correct_ground_intersection(self, tunnel: tunnel) -> None:
        """Coloca o player precisamente acima do chão após uma queda.

        Args:
            tunnel: objeto capaz de retornar a interseção entre o rect inferior do player e o rect do chão.
        """
        intersection_rect = tunnel.return_ground_intersection(self.rect_down)
        if intersection_rect.height > 1:
            self.update_position(0, -intersection_rect.height+1)
            print(f"Interseção corrigida: {intersection_rect.height} pixels ajustados.")
    
    def is_colliding(self, tunnel: tunnel) -> bool:
        """Retorna True se o robo estiver colidindo com o solo
        
        Args:
            tunnel: objeto que contém função capaz de checar colisões 
        """
        return tunnel.check_collision(self.rect_down)
    
    def animate(self) -> None:
        self.is_animating = True
    
    #TODO: FAZER A DOCSTRING
    def update(self) -> None:
        """Atualiza 
        """
        self.image = self.__sprites_idle[int(self.current_sprite)]
                
    def draw_collision_rect(self, screen: pygame.display) -> None:
        """Desenha os rects do player na tela.

        Função com uso estrito para testes relacionados aos rects do player.
        """
        # Desenha um retângulo vermelho em torno do retângulo do jogador
        green = (0, 255, 0)
        red = (255, 0, 0)
        white = (255, 255, 255)
        black = (0,0,0)
        pygame.draw.rect(screen, green, self.rect, 1)
        pygame.draw.rect(screen, black, self.rect_down, 1)