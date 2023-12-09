/*
 * Computacao Grafica
 * Codigo Exemplo: Rasterizacao de Segmentos de Reta com GLUT/OpenGL
 * Autor: Prof. Laurindo de Sousa Britto Neto
 */

// Bibliotecas utilizadas pelo OpenGL
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

#include <cmath>
#include <stack>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <forward_list>
#include "glut_text.h"

using namespace std;

// Variaveis Globais
#define ESC 27
#define ENTER 13
#define DEL 127
#define letra_d 100
#define letra_a 97
#define letra_s 115
#define letra_w 119

//Enumeracao com os tipos de formas geometricas
enum tipo_transf{TRNSLA = 1, ESCL, CIS, REFLX, ROT};
enum tipo_forma{LIN = 1, QUAD, TRI, POL, CIR, TRANSF, FLOOD}; // Linha, Triangulo, Poligono, Circulo

//Verifica se foi realizado o primeiro clique do mouse
bool click1 = false;

//Verifica se foi realizado o segundo clique do mouse
bool click2 = false;

//Verifica se foi realizado o terceiro clique do mouse
bool click3 = false;

int numVertices = 4;
int cliques = 0;

//Coordenadas da posicao atual do mouse
int m_x, m_y;

//Coordenadas do primeiro clique, do segundo clique e do terceiro clique do mouse
int x_1, y_1, x_2, y_2, x_3, y_3, x_4, y_4;
// Guarda as cordenadas do clique inicial do polígono
int x_0, y_0;

//Lista de vértices para o polígono
forward_list<pair<int, int>> vertices;

//Indica o tipo de forma geometrica ativa para desenhar
int modo = LIN;
int transformacao;

// Define o ângulo da rotação
int angulo = 0;

//Largura e altura da janela
int width = 512, height = 512;

// Definicao de vertice
struct vertice{
    int x;
    int y;
};

// Definicao das formas geometricas
struct forma{
    int tipo;
    forward_list<vertice> v; //lista encadeada de vertices
};

// Calcular centroide
vertice calcularCentroide(forward_list<vertice>& pontos) {
    vertice centroide = {0, 0};
    int numPontos = 0;

    for (forward_list<vertice>::iterator it = pontos.begin(); it != pontos.end(); ++it) {
        centroide.x += it->x;
        centroide.y += it->y;
        numPontos++;
	}

    if (numPontos > 0) {
        centroide.x = round(centroide.x/numPontos);
        centroide.y = round(centroide.y/numPontos);
    }

    return centroide;
}

vertice calcularPontoOrigem(forward_list<vertice>& pontos, bool eixoX){
	if(eixoX){
		vertice pontoOrigem = pontos.front();
			for (forward_list<vertice>::iterator it = pontos.begin(); it != pontos.end(); ++it) {
	        	if(it->y < pontoOrigem.y){
					pontoOrigem = *it;
				}
			}
			return pontoOrigem;
	}else{
			vertice pontoOrigem = pontos.front();
			for (forward_list<vertice>::iterator it = pontos.begin(); it != pontos.end(); ++it) {
	        	if(it->x < pontoOrigem.x){
					pontoOrigem = *it;
				}
			}
			return pontoOrigem;
	}
}

// Defino um ponteiro para a última forma salva no programa
forma* ultimaForma;

struct Cor {
    float r, g, b;
};

struct Cor getPixelColor(int x, int y) {
	struct Cor cor;
	glReadPixels(x, y, 1, 1, GL_RGB, GL_FLOAT, &cor);
	return cor;
}

void setPixelColor(int x, int y,struct Cor cor) {
	glBegin(GL_POINTS);
	glColor3f(0.0, 0.0, 0.0);
	glVertex2i(x, y);
	glEnd();
	//glFlush();
}

// Marca os pixels já visitados para o algoritmo floodfill
vector<vector<bool>> pixelsVisitados(width, vector<bool>(height, false));

// Função para verificar se duas cores são iguais
bool coresIguais(const Cor& c1, const Cor& c2) {
    return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b;
}

// Função para adicionar um vértice à lista
void addVertice(int x, int y) {
    vertices.push_front(make_pair(x, y));
}

// Lista encadeada de formas geometricas
forward_list<forma> formas;

// Funcao para armazenar uma forma geometrica na lista de formas
// Armazena sempre no inicio da lista
void pushForma(int tipo){
    forma f;
    f.tipo = tipo;
    formas.push_front(f);
    ultimaForma = &formas.front();
}

// Funcao para armazenar um vertice na forma do inicio da lista de formas geometricas
// Armazena sempre no inicio da lista
void pushVertice(int x, int y){
    vertice v;
    v.x = x;
    v.y = y;
    formas.front().v.push_front(v);
}

//Fucao para armazenar uma Linha na lista de formas geometricas
void pushLinha(int x1, int y1, int x2, int y2){
    pushForma(LIN);
    pushVertice(x1, y1);
    pushVertice(x2, y2);
}

void pushQuadrilatero(int x1, int y1, int x2, int y2){
    pushForma(QUAD);
    pushVertice(x1, y1);
    pushVertice(x1, y2);
    pushVertice(x2, y2);
    pushVertice(x2, y1);
}

void pushTriangulo(int x1, int y1, int x2, int y2, int x3, int y3){
    pushForma(TRI);
    pushVertice(x1, y1);
    pushVertice(x2, y2);
    pushVertice(x3, y3);
}

void pushPoligono(){
	pushForma(POL);
    for(auto it = vertices.begin(); it != vertices.end(); ++it){
		pair<int, int> v = *it;
		pushVertice(v.first, v.second);
	}
	vertices.clear();
}

void pushCircunferencia(int cx, int cy, int rx, int ry){
    pushForma(CIR);
    pushVertice(cx, cy);  
    pushVertice(rx, ry);  
}

/*
 * Declaracoes antecipadas (forward) das funcoes (assinaturas das funcoes)
 */
void init(void);
void reshape(int w, int h);
void display(void);
void menu_popup(int value);
void submenu_transf(int value);
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void mousePassiveMotion(int x, int y);
void drawPixel(int x, int y);
// Funcao que percorre a lista de formas geometricas, desenhando-as na tela
void drawFormas();
// Funcao que implementa o Algoritmo Imediato para rasterizacao de segmentos de retas
void retaImediata(double x1,double y1,double x2,double y2);
void bresenham(double x1,double y1,double x2,double y2);
void quadrilatero(int x1, int y1, int x2, int y2);
void quadrilatero(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
void triangulo(int x1, int y1, int x2, int y2, int x3, int y3);
void poligono(forward_list<vertice> v);
void circunferencia(int cx, int cy, int raio);
void floodFill(int x1, int x2, Cor cor, Cor novaCor);
void translacao(int tx, int ty);
void rotacao(int angulo);
void escala(float sx, float sy);
void reflexao(bool eixoX, bool eixoY);
void cisalhamento(float cx, float cy);

/*
 * Funcao principal
 */
int main(int argc, char** argv){
    glutInit(&argc, argv); // Passagens de parametro C para o glut
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB); //Selecao do Modo do Display e do Sistema de cor
    glutInitWindowSize (width, height);  // Tamanho da janela do OpenGL
    glutInitWindowPosition (100, 100); //Posicao inicial da janela do OpenGL
    glutCreateWindow ("Computacao Grafica: Paint"); // Da nome para uma janela OpenGL
    init(); // Chama funcao init();
    glutReshapeFunc(reshape); //funcao callback para redesenhar a tela
    glutKeyboardFunc(keyboard); //funcao callback do teclado
    glutMouseFunc(mouse); //funcao callback do mouse
    glutPassiveMotionFunc(mousePassiveMotion); //fucao callback do movimento passivo do mouse
    glutDisplayFunc(display); //funcao callback de desenho
    
  //  Criar submenu de transformações
    int submenuTransf = glutCreateMenu(submenu_transf);
    glutAddMenuEntry("Translação", TRNSLA);
    glutAddMenuEntry("Escala", ESCL);
    glutAddMenuEntry("Cisalhamento", CIS);
    glutAddMenuEntry("Reflexão", REFLX);
    glutAddMenuEntry("Rotação", ROT);
    
    // Define o menu pop-up
    glutCreateMenu(menu_popup);
    glutAddMenuEntry("Linha", LIN);
   // glutAddMenuEntry("Retangulo", RET);
	glutAddMenuEntry("Quadrilátero", QUAD);
	glutAddMenuEntry("Triângulo", TRI);
	glutAddMenuEntry("Polígono", POL);
	glutAddMenuEntry("Circunferência", CIR);
	glutAddMenuEntry("Balde de Tinta", FLOOD);
	// Anexar submenu de transformações ao menu principal
    glutAddSubMenu("Transformações", submenuTransf);
    glutAddMenuEntry("Sair", 0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    glutMainLoop(); // executa o loop do OpenGL
    return EXIT_SUCCESS; // retorna 0 para o tipo inteiro da funcao main();
}

/*
 * Inicializa alguns parametros do GLUT
 */
void init(void){
    glClearColor(1.0, 1.0, 1.0, 1.0); //Limpa a tela com a cor branca;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
  	gluOrtho2D(0 , 512 , 0 , 512);
}

/*
 * Ajusta a projecao para o redesenho da janela
 */
void reshape(int w, int h)
{
	// Muda para o modo de projecao e reinicializa o sistema de coordenadas
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Definindo o Viewport para o tamanho da janela
	glViewport(0, 0, w, h);
	
	width = w;
	height = h;
    glOrtho (0, w, 0, h, -1 ,1);  

   // muda para o modo de desenho
	glMatrixMode(GL_MODELVIEW);
 	glLoadIdentity();

}

/*
 * Controla os desenhos na tela
 */
void display(void){
    glClear(GL_COLOR_BUFFER_BIT); //Limpa o buffer de cores e reinicia a matriz
    glColor3f (0.0, 0.0, 0.0); // Seleciona a cor default como preto
    drawFormas(); // Desenha as formas geometricas da lista
    //Desenha texto com as coordenadas da posicao do mouse
    draw_text_stroke(0, 0, "(" + to_string(m_x) + "," + to_string(m_y) + ")", 0.2);
    glutSwapBuffers(); // manda o OpenGl renderizar as primitivas

}

/*
 * Controla o menu pop-up
 */
void menu_popup(int value){
    if (value == 0) exit(EXIT_SUCCESS);
    if (modo == TRANSF && transformacao == ROT) angulo = 0;
    modo = value;
	glutPostRedisplay();
}

void submenu_transf(int value){
    if (value == 0) exit(EXIT_SUCCESS);
    if (transformacao == ROT) angulo = 0;
    modo = TRANSF;
    transformacao = value;
	glutPostRedisplay();
}

/*
 * Controle das teclas comuns do teclado
 */
void keyboard(unsigned char key, int x, int y){
    switch (key) { // key - variavel que possui valor ASCII da tecla precionada
        case ESC: exit(EXIT_SUCCESS);
			break;
		case ENTER:
			switch(modo){
        		case POL:
	            	if(distance(vertices.begin(), vertices.end()) > 1){
	            		x_1 = x;
	            		y_1 = height - y - 1;
	            		addVertice(x_1, y_1);
	            		cliques = 0;
	            		pushPoligono();
	               	    glutPostRedisplay();
		            }
		            break;
			}
			break;
		case DEL:
			formas.clear();
			break;
		case letra_a:
			if(modo == TRANSF){
				switch(transformacao){
					case TRNSLA:
						translacao(-10, 0);
						break;
					case ROT:
						rotacao((angulo+5)%360);
						break;
					case ESCL:
						escala(0.8, 1.0);
						break;
					case CIS:
						cisalhamento(-0.5, 0.0);
						break;
				}
			}
			break;
		case letra_d:
			if(modo == TRANSF){
				switch(transformacao){
					case TRNSLA:
						translacao(10, 0);
						break;
					case ROT:
						rotacao((angulo-5)%360);
						break;
					case ESCL:
						escala(1.2, 1.0);
						break;
					case REFLX:
						reflexao(false, true);
						break;
					case CIS:
						cisalhamento(0.5, 0.0);
						break;
				}
			}
			break;
		case letra_w:
			if(modo == TRANSF){
				switch(transformacao){
					case TRNSLA:
						translacao(0, 10);
						break;
					case ESCL:
						escala(1.0, 1.2);
						break;
					case REFLX:
						reflexao(true, false);
						break;
					case CIS:
						cisalhamento(0.0, 0.5);
						break;
				}
			}
			break;
		case letra_s:
			if(modo == TRANSF){
				switch(transformacao){
					case TRNSLA:
						translacao(0, -10);
						break;
					case ESCL:
						escala(1, 0.8);
						break;
					case CIS:
						cisalhamento(0.0, -0.5);
						break;
				}
			}
			break;
    }
}

/*
 * Controle dos botoes do mouse
 */
void mouse(int button, int state, int x, int y){
    switch (button) {
        case GLUT_LEFT_BUTTON:
            switch(modo){
                case LIN:
                    if (state == GLUT_DOWN) {
                        if(click1){
                            x_2 = x;
                            y_2 = height - y - 1;
                            printf("Clique 2(%d, %d)\n",x_2,y_2);
                            pushLinha(x_1, y_1, x_2, y_2);
                            click1 = false;
                            glutPostRedisplay();
                        }else{
                            click1 = true;
                            x_1 = x;
                            y_1 = height - y - 1;
                            printf("Clique 1(%d, %d)\n",x_1,y_1);
                        }
                    }
                	break;
                 case QUAD:
                    if (state == GLUT_DOWN) {
      //               	if (click1 && click2 && click3) {
      //                       x_4 = x;
      //                       y_4 = height - y - 1;
      //                       printf("Clique 2(%d, %d)\n", x_2, y_2);
      //                       pushQuadrilatero(x_1, y_1, x_2, y_2, x_3, y_3, x_4, y_4);
      //                       click1 = click2 = click3 = false;
      //                       glutPostRedisplay();
						// }else if (click1 && click2) {
      //                       x_3 = x;
      //                       y_3 = height - y - 1;
      //                       printf("Clique 2(%d, %d)\n", x_2, y_2);
      //                       click3 = true;
						if (click1) {
                            x_2 = x;
                            y_2 = height - y - 1;
                            printf("Clique 2(%d, %d)\n", x_2, y_2);
                            pushQuadrilatero(x_1, y_1, x_2, y_2);
                            click1 = false;
                        } else {
                            click1 = true;
                            x_1 = x;
                            y_1 = height - y - 1;
                            printf("Clique 1(%d, %d)\n", x_1, y_1);
                        }
                    }
                    break;
                case TRI:
                    if (state == GLUT_DOWN) {
                    	if(click1 && click2){
							x_3 = x;
							y_3 = height - y - 1;
							printf("Clique 3(%d, %d)\n", x_3, y_3);
							pushTriangulo(x_1, y_1, x_2, y_2, x_3, y_3);
							click1 = click2 = false;
							glutPostRedisplay();
						}else if (click1) {
                            x_2 = x;
                            y_2 = height - y - 1;
                            printf("Clique 2(%d, %d)\n", x_2, y_2);
                            click2 = true;
                        } else {
                            click1 = true;
                            x_1 = x;
                            y_1 = height - y - 1;
                            printf("Clique 1(%d, %d)\n", x_1, y_1);
                        }
                    }
                    break;
                case POL:
                	if (state == GLUT_DOWN){
						x_1 = x;
						y_1 = height - y - 1;
						cliques += 1;
						if (cliques == 1){
							x_0 = x_1;
							y_0 = y_1;
						}
						addVertice(x, height - y - 1);
						glutPostRedisplay();
					}
					break;
				case CIR:
				    if (state == GLUT_DOWN) {
				        if (click1) {
				            click1 = false;
				            x_2 = x;
				            y_2 = height - y - 1;
				            printf("Clique 2(%d, %d)\n", x_2, y_2);
				            pushCircunferencia(x_1, y_1, x_2, y_2);
				            glutPostRedisplay();
				        } else {
				            click1 = true;
				            x_1 = x;
				            y_1 = height - y - 1;
				            printf("Clique 1(%d, %d)\n", x_1, y_1);
				        }
				    }
				    break;
				case FLOOD:
					if (state == GLUT_DOWN){
						x_1 = x;
						y_1 = height - y - 1;
						Cor cor, novaCor;
						cor.r = 1.0, cor.g = 1.0, cor.b = 1.0;
						novaCor.r = 0.0, novaCor.g = 0.0, novaCor.b = 0.0;
						floodFill(x_1, y_1, cor, novaCor);
						glutPostRedisplay();
						glFlush();
					}
					break;
			}
			break;
        case GLUT_MIDDLE_BUTTON:
			switch(modo){
        		case POL:
		            if (state == GLUT_DOWN) {
		            	if(distance(vertices.begin(), vertices.end()) > 1){
		            		x_1 = x;
		            		y_1 = height - y - 1;
		            		addVertice(x_1, y_1);
		            		cliques = 0;
		            		pushPoligono();
		               	    glutPostRedisplay();
						}
		            }
			}
	        break;
        case GLUT_RIGHT_BUTTON:
        	  // Lógica para o botão direito
            switch (modo) {
                case POL:
                    if (state == GLUT_DOWN) {
                        if (distance(vertices.begin(), vertices.end()) > 1) {
                            x_1 = x;
		            		y_1 = height - y - 1;
		            		addVertice(x_1, y_1);
		            		cliques = 0;
		            		pushPoligono();
		               	    glutPostRedisplay();
                        }
                    }
                    break;
            }
        	break;
	}
}

/*
 * Controle da posicao do cursor do mouse
 */
void mousePassiveMotion(int x, int y){
    m_x = x; m_y = height - y - 1;
    glutPostRedisplay();
}

/*
 * Funcao para desenhar apenas um pixel na tela
 */
void drawPixel(int x, int y){
    glBegin(GL_POINTS); // Seleciona a primitiva GL_POINTS para desenhar
        glVertex2i(x, y);
    glEnd();  // indica o fim do ponto
}

/*
 *Funcao que desenha a lista de formas geometricas
 */
void drawFormas() {
    // Após o primeiro clique, desenha a reta com a posição atual do mouse
    if(click1 && click2 && modo == TRI){
		bresenham(x_1, y_1, x_2, y_2);
		bresenham(x_2, y_2, m_x, m_y);
	}if (click1 && modo == TRI){
        bresenham(x_1, y_1, m_x, m_y);
	}else if (click1 && modo == QUAD){
		quadrilatero(x_1, y_1, m_x, m_y);
	}else if(click1 && modo == LIN){
		bresenham(x_1, y_1, m_x, m_y);
	}else if(click1 && modo == CIR){
		int raio = sqrt(pow((x_1 - m_x), 2) + pow((y_1 - m_y), 2));
		circunferencia(x_1, y_1, raio);
	}else if(cliques > 0 && modo == POL){
		bresenham(x_1, y_1, m_x, m_y);
		bresenham(x_0, y_0, m_x, m_y);
		if (distance(vertices.begin(), vertices.end()) > 1) {
	        auto it = vertices.begin();
	        pair<int, int> prev = *it;
	        ++it;
	        while (it != vertices.end()){
	            pair<int, int> current = *it;
	            bresenham(prev.first, prev.second, current.first, current.second);
	            prev = current;
	            ++it;
			}
		}	
	}

    // Percorre a lista de formas geométricas para desenhar
    for (forward_list<forma>::iterator f = formas.begin(); f != formas.end(); f++) {
        switch (f->tipo) {
            case LIN: {
                // Percorre a lista de vertices da forma linha para desenhar
                int i = 0, x[2], y[2];
                for (forward_list<vertice>::iterator v = f->v.begin(); v != f->v.end(); v++, i++) {
                    x[i] = v->x;
                    y[i] = v->y;
                }
                // Desenha o segmento de reta após dois cliques
                bresenham(x[0], y[0], x[1], y[1]);
                break;
            }
            case QUAD: {
                // Percorre a lista de vertices da forma quadrilátero para desenhar
                int i = 0, x[4], y[4];
                for (forward_list<vertice>::iterator v = f->v.begin(); v != f->v.end(); v++, i++) {
                    x[i] = v->x;
                    y[i] = v->y;
                }
                // Desenha o quadrilátero
                quadrilatero(x[0], y[0], x[1], y[1], x[2], y[2], x[3], y[3]);
                break;
            }
            case TRI: {
                // Percorre a lista de vertices da forma triângulo para desenhar
                int i = 0, x[3], y[3];
                for (forward_list<vertice>::iterator v = f->v.begin(); v != f->v.end(); v++, i++) {
                    x[i] = v->x;
                    y[i] = v->y;
                }
                // Desenha o triângulo
                triangulo(x[0], y[0], x[1], y[1], x[2], y[2]);
                break;
            }
            case POL: {
				poligono(f->v);
				break;
			}
			case CIR:{
				int i = 0, x[2], y[2];
                for (forward_list<vertice>::iterator v = f->v.begin(); v != f->v.end(); v++, i++) {
                    x[i] = v->x;
                    y[i] = v->y;
                }
                int raio = sqrt(pow(x[0] - x[1], 2) + pow(y[0] - y[1], 2));
				circunferencia(x[1], y[1], raio);
				break;
			}
        }
    }
}


/*
 * Fucao que implementa o Algoritmo de Rasterizacao da Reta Imediata
 */
void retaImediata(double x1, double y1, double x2, double y2){
    double m, b, yd, xd;
    double xmin, xmax,ymin,ymax;
    
    drawPixel((int)x1,(int)y1);
    if(x2-x1 != 0){ //Evita a divisao por zero
        m = (y2-y1)/(x2-x1);
        b = y1 - (m*x1);

        if(m>=-1 && m <= 1){ // Verifica se o declive da reta tem tg de -1 a 1, se verdadeira calcula incrementando x
            xmin = (x1 < x2)? x1 : x2;
            xmax = (x1 > x2)? x1 : x2;

            for(int x = (int)xmin+1; x < xmax; x++){
                yd = (m*x)+b;
                yd = floor(0.5+yd);
                drawPixel(x,(int)yd);
            }
        }else{ // Se tg menor que -1 ou maior que 1, calcula incrementado os valores de y
            ymin = (y1 < y2)? y1 : y2;
            ymax = (y1 > y2)? y1 : y2;

            for(int y = (int)ymin + 1; y < ymax; y++){
                xd = (y - b)/m;
                xd = floor(0.5+xd);
                drawPixel((int)xd,y);
            }
        }

    }else{ // se x2-x1 == 0, reta perpendicular ao eixo x
        ymin = (y1 < y2)? y1 : y2;
        ymax = (y1 > y2)? y1 : y2;
        for(int y = (int)ymin + 1; y < ymax; y++){
            drawPixel((int)x1,y);
        }
    }
    drawPixel((int)x2,(int)y2);
}

void bresenham(double x1,double y1,double x2,double y2){
	bool declive, simetrico;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int x , y;
    
    //Redução de octante
    declive = false;
    simetrico = false;
    if ((dx * dy) < 0){
        y1 = (-1) * y1;
        y2 = (-1) * y2;
        dy = (-1) * dy;
        simetrico = true;
    }
    if (abs(dx) < abs(dy)){
        swap(x1, y1);
        swap(x2, y2);
        swap(dx, dy);
        declive = true;
    }

    if(x1 > x2){
        swap(x1, x2);
        swap(y1, y2);
        dx = (-1) * dx;
        dy = (-1) * dy;
    }
    
    x = x1;
    y = y1;
    int x_;
    int y_;

    //Algoritmo de Bresenham
    int d = (2 * dy) - dx;
    int incE = 2 * dy;
    int incNE = 2 * (dy - dx);
    
    while(x < x2){
        //incE
        if(d <= 0){
            d += incE;
        }else{ //incNE
            d += incNE;
            y += 1;
        }
        x += 1;
        
        //Transformação para o octante original
        x_ = x;
        y_ = y;
        if(declive == true){
            x_ = y;
            y_ = x;
        }

        if(simetrico == true){
            y_ = (-1) * y_;
        }
        drawPixel(x_, y_);
    }
}

void quadrilatero(int x1, int y1, int x2, int y2) {
    bresenham(x1, y1, x1, y2);
    bresenham(x1, y2, x2, y2);
    bresenham(x2, y2, x2, y1);
    bresenham(x2, y1, x1, y1);
}

void quadrilatero(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
    bresenham(x1, y1, x2, y2);
    bresenham(x2, y2, x3, y3);
    bresenham(x3, y3, x4, y4);
    bresenham(x4, y4, x1, y1);
}

void triangulo(int x1, int y1, int x2, int y2, int x3, int y3){
	bresenham(x1, y1, x2, y2);
	bresenham(x2, y2, x3, y3);
	bresenham(x3, y3, x1, y1);
}

void poligono(forward_list<vertice> v){
	auto it = v.begin();
	vertice ant = *it;
	vertice prim = ant;
	it++;
	for (; it != v.end(); ++it){
		bresenham(ant.x, ant.y, (*it).x, (*it).y);
        ant = (*it);
	}
	bresenham(ant.x, ant.y, prim.x, prim.y);
}


void circunferencia(int cx, int cy, int raio) {
    int d = 1 - raio;
    int dE = 3;
    int dSE = ((-2) * raio) + 5;
    int x = 0, y = raio;
    
    drawPixel(cx, raio + cy);
    drawPixel(cx, -raio + cy);
    drawPixel(raio + cx, cy);
    drawPixel(-raio + cx, cy);

    while (y > x) {
        if (d < 0) {
            d += dE;
            dE += 2;
            dSE += 2;
        } else {
            d += dSE;
            dE += 2;
            dSE += 4;
            y -= 1;
        }
        
        x += 1;
        
        drawPixel(x + cx, y + cy);
        drawPixel(-x + cx, y + cy);
        drawPixel(x + cx, -y + cy);
        drawPixel(-x + cx, -y + cy);
        
        if (x != y) {
            drawPixel(y + cx, x + cy);
            drawPixel(-y + cx, x + cy);
            drawPixel(y + cx, -x + cy);
            drawPixel(-y + cx, -x + cy);
        }
    }
}

void floodFill(int x, int y, Cor old_col, Cor new_col) {
    std::stack<std::pair<int, int>> pilha;
    pilha.push(std::make_pair(x, y));

    while (!pilha.empty()) {
        std::pair<int, int> coordenadas = pilha.top();
        pilha.pop();
        x = coordenadas.first;
        y = coordenadas.second;
        
        if (x < 0 || x >= width || y < 0 || y >= height || pixelsVisitados[x][y]) {
            continue;
        }

        pixelsVisitados[x][y] = true;

        Cor corAtual = getPixelColor(x, y);

        if (corAtual.r == new_col.r && corAtual.g == new_col.g && corAtual.b == new_col.b) {
            continue;
        }

        if (corAtual.r == old_col.r && corAtual.g == old_col.g && corAtual.b == old_col.b) {
            drawPixel(x, y);

            pilha.push(std::make_pair(x + 1, y));
            pilha.push(std::make_pair(x - 1, y));
            pilha.push(std::make_pair(x, y + 1));
            pilha.push(std::make_pair(x, y - 1));
        }
    }
}

void translacao(int tx, int ty){
	if (!formas.empty()){
		for (forward_list<vertice>::iterator it = ultimaForma->v.begin(); it != ultimaForma->v.end(); ++it) {
            it->x = it->x + tx;
            it->y = it->y + ty;
        }
	}
	glutPostRedisplay();
}

void rotacao(int angulo){
	if (!formas.empty()){
		vertice centroide = calcularCentroide(ultimaForma->v);
		float rad = angulo * 3.14159 / 180;
		for (forward_list<vertice>::iterator it = ultimaForma->v.begin(); it != ultimaForma->v.end(); ++it) {
            // Translada para a origem
            it->x -= centroide.x;
            it->y -= centroide.y;

            // Rotaciona
            int novoX = round((it->x * cos(rad)) - (it->y * sin(rad)));
            int novoY = round((it->x * sin(rad)) + (it->y * cos(rad)));

            // Atualiza as coordenadas
            it->x = novoX;
            it->y = novoY;

            // Translada de volta para a posição original
            it->x += centroide.x;
            it->y += centroide.y;
        }
	}
	glutPostRedisplay();
}

void escala(float sx, float sy){
	if (!formas.empty()){
		vertice origem;
		
		if(sx == 1) origem = calcularPontoOrigem(ultimaForma->v, true);
		if(sy == 1) origem = calcularPontoOrigem(ultimaForma->v, false);
		for (forward_list<vertice>::iterator it = ultimaForma->v.begin(); it != ultimaForma->v.end(); ++it) {
            // Translada para a origem
            it->x -= origem.x;
            it->y -= origem.y;

            // Escala
            int novoX, novoY;
            novoX = float(it->x)*sx;
            novoY = float(it->y)*sy;

            // Atualiza as coordenadas
            it->x = novoX;
            it->y = novoY;

            // Translada de volta para a posição original
            it->x += origem.x;
            it->y += origem.y;
        }
	}
	glutPostRedisplay();
}

void reflexao(bool eixoX, bool eixoY){
	if(eixoX){
		if (!formas.empty()){
			vertice centroide = calcularCentroide(ultimaForma->v);
			
			for (forward_list<vertice>::iterator it = ultimaForma->v.begin(); it != ultimaForma->v.end(); ++it) {
	            // Translada para a origem
	            it->x -= centroide.x;
	            it->y -= centroide.y;
	
	            // Reflete
				int novoY = it->y*(-1);
	
	            // Atualiza as coordenadas
	            it->y = novoY;
	
	            // Translada de volta para a posição original
	            it->x += centroide.x;
	            it->y += centroide.y;
	        }
		}
	}else{
		if (!formas.empty()){
			vertice centroide = calcularCentroide(ultimaForma->v);
			
			for (forward_list<vertice>::iterator it = ultimaForma->v.begin(); it != ultimaForma->v.end(); ++it) {
	            // Translada para a origem
	            it->x -= centroide.x;
	            it->y -= centroide.y;
	
	            // Reflete
				int novoX = it->x*(-1);
	
	            // Atualiza as coordenadas
	            it->x = novoX;
	
	            // Translada de volta para a posição original
	            it->x += centroide.x;
	            it->y += centroide.y;
	        }
		}
	}
	glutPostRedisplay();
}

void cisalhamento(float cx, float cy){
		if (!formas.empty()){
			vertice origem;
		
			if(cx == 0) origem = calcularPontoOrigem(ultimaForma->v, true);
		   	if(cy == 0) origem = calcularPontoOrigem(ultimaForma->v, false);
			for (forward_list<vertice>::iterator it = ultimaForma->v.begin(); it != ultimaForma->v.end(); ++it) {
	            // Translada para a origem
	            it->x -= origem.x;
	            it->y -= origem.y;
	
	            // Cisalha
	            int novoX = it->x + it->y*cx;
				int novoY = it->y + it->x*cy;
	
	            // Atualiza as coordenadas
	            it->x = novoX;
	            it->y = novoY;
	
	            // Translada de volta para a posição original
	            it->x += origem.x;
	            it->y += origem.y;
	        }
		}
		glutPostRedisplay();
}