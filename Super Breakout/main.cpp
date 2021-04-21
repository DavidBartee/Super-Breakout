#include <iostream>
#include <utility>
#include <SDL.h>
#include <SDL_mixer.h>
#ifndef CLEANUP_H
#define CLEANUP_H
template<typename T, typename... Args>
void cleanup(T* t, Args&&... args) {
	cleanup(t);
	cleanup(std::forward<Args>(args)...);
}
template<>
inline void cleanup<SDL_Window>(SDL_Window* win) {
	if (!win) {
		return;
	}
	SDL_DestroyWindow(win);
}
template<>
inline void cleanup<SDL_Renderer>(SDL_Renderer* ren) {
	if (!ren) {
		return;
	}
	SDL_DestroyRenderer(ren);
}
template<>
inline void cleanup<SDL_Texture>(SDL_Texture* tex) {
	if (!tex) {
		return;
	}
	SDL_DestroyTexture(tex);
}
template<>
inline void cleanup<SDL_Surface>(SDL_Surface* surf) {
	if (!surf) {
		return;
	}
	SDL_FreeSurface(surf);
}
#endif
const int WIDTH = 1000, HEIGHT = 1000;
const int bricksWidth = 13, bricksHeight = 28, widthPadding = 2, heightPadding = 8;
const float paddleWidthStart = 1.5 / (bricksWidth + (float)widthPadding);//Paddle width is represented as a percentage of the screen's width
float paddleWidth = paddleWidthStart;
const float paddleHeight = (1.0 / ((float)bricksHeight + (float)heightPadding));
float paddleX = 0.5;//also a percentage
int brickStates[bricksWidth][bricksHeight];
int lives = 5;
const float ballRespawnTime = 1.0f;
float ballRespawnTimer = 0;
bool ballMissed = false;
bool gamePaused = false;
//Ball info
float ballSize = paddleHeight * 0.6f;
const float ballSpeedStart = 0.5;//Percent of the screen per unit of time
float ballSpeed = ballSpeedStart;//This is the sum of the X and Y speed
float ballSpeedX = 0;
float ballSpeedY = ballSpeedStart / 2.0f;
const float BALL_SPEED_MULT_CAP = 1.5f;//Limit how much the speed can be multiplied
const float ballCooldown = 0.05f;//Limit how often the ball can break a brick
float lastBreakTime = 0;
float ballX = 0.5f;
//Start the ball near the paddle
float ballY = 0.7f;
int paddleHitCounter = 0;
float lastPaddleHitTime = 0;
int lineCounter = 0;
const float lineCooldown = 0.1f;
float lastLineTime = 0;
//Sound effects
const char* wavBoop = "Sounds/atari boop.wav";
const char* wavLine = "Sounds/atari line.wav";
const char* wavBrick = "Sounds/brick1.wav";
const char* wavBrick2 = "Sounds/brick2.wav";
const char* wavBrick3 = "Sounds/brick3.wav";
const char* wavBrick4 = "Sounds/brick6.wav";
int brickSoundCounter = 0;
const int numBrickSounds = 8;
//Scoring
int score = 0;
int scoreQueue = 0;
const float scoreQueueInterval = 1.0f / 10.0f;
float scoreQueueTimer = 0;
const float scoreMargin = 0.12f;//Percent of the screen's height used by the score info on top
//Digit images
std::string digitPaths[] = {
	"Images/0.bmp",
	"Images/1.bmp",
	"Images/2.bmp",
	"Images/3.bmp",
	"Images/4.bmp",
	"Images/5.bmp",
	"Images/6.bmp",
	"Images/7.bmp",
	"Images/8.bmp",
	"Images/9.bmp"
};
SDL_Texture* digitImg[10];
const int SCORE_DIGITS = 5;

void logSDLError(std::ostream& os, const std::string& msg) {
	os << msg << " error: " << SDL_GetError() << std::endl;
}

SDL_Texture* loadTexture(const std::string& file, SDL_Renderer* ren) {
	SDL_Texture* texture = nullptr;
	SDL_Surface* loadedImage = SDL_LoadBMP(file.c_str());
	if (loadedImage != nullptr) {
		texture = SDL_CreateTextureFromSurface(ren, loadedImage);
		SDL_FreeSurface(loadedImage);
		if (texture == nullptr) {
			logSDLError(std::cout, "CreateTextureFromSurface");
		}
	} else {
		logSDLError(std::cout, "LoadBMP");
	}
	return texture;
}

void renderTexture(SDL_Texture* tex, SDL_Renderer* ren, int x, int y, int w, int h) {
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	dst.w = w;
	dst.h = h;
	SDL_RenderCopy(ren, tex, NULL, &dst);
}

void renderTexture(SDL_Texture* tex, SDL_Renderer* ren, int x, int y) {
	int w, h;
	SDL_QueryTexture(tex, NULL, NULL, &w, &h);
	renderTexture(tex, ren, x, y, w, h);
}

void printStates() {
	for (int i = 0; i < bricksWidth; i++) {
		for (int j = 0; j < bricksHeight - 7; j++) {
			std::cout << brickStates[i][j] << " ";
		}
		std::cout << std::endl;
	}
}

void resetGame() {
	lives = 5;
	ballMissed = false;
	ballSpeed = ballSpeedStart;
	ballSpeedX = 0;
	ballSpeedY = ballSpeedStart / 2.0f;
	ballX = 0.5f;
	ballY = 0.7f;
	paddleX = 0.5f;
	paddleWidth = paddleWidthStart;
	paddleHitCounter = 0;
	lineCounter = 0;
	brickSoundCounter = 0;
	score = 0;
	scoreQueue = 0;
	scoreQueueTimer = 0;
	//Initialize bricks
	for (int i = 0; i < bricksWidth; i++) {
		for (int j = 0; j < bricksHeight - 7; j++) {
			if ((j) % 8 < 4/* && (i + j) % 2 == 0*/) {
				brickStates[i][j] = 1;
			} else {
				brickStates[i][j] = 0;
			}
		}
	}
}

int main(int argc, char* argv[]) {
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		logSDLError(std::cout, "SDL_Init");
		return 1;
	}
	SDL_ShowCursor(SDL_DISABLE);
	//Create window
	SDL_Window* window = SDL_CreateWindow("Super Breakout", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
	if (window == nullptr) {
		logSDLError(std::cout, "CreateWindow");
		SDL_Quit();
		return 1;
	}
	//Create renderer
	SDL_Renderer* ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (ren == nullptr) {
		logSDLError(std::cout, "CreateRenderer");
		cleanup(window);
		SDL_Quit();
		return 1;
	}
	//Load BMPs
	for (int i = 0; i < 10; i++) {
		digitImg[i] = loadTexture(digitPaths[i], ren);
		if (digitImg[i] == nullptr) {
			cleanup(ren, window);
			logSDLError(std::cout, "LoadBMP");
			SDL_Quit();
			return 1;
		}
	}
	//Load audio file
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		std::cout << "Error: " << Mix_GetError() << std::endl;
	}
	Mix_Chunk* boopSound = Mix_LoadWAV(wavBoop);
	Mix_Chunk* lineSound = Mix_LoadWAV(wavLine);
	Mix_Chunk* brickSound = Mix_LoadWAV(wavBrick);
	Mix_Chunk* brickSound2 = Mix_LoadWAV(wavBrick2);
	Mix_Chunk* brickSound3 = Mix_LoadWAV(wavBrick3);
	Mix_Chunk* brickSound4 = Mix_LoadWAV(wavBrick4);

	resetGame();
	SDL_Event e;
	bool quit = false;
	float mouseMotion = 0;
	SDL_SetRelativeMouseMode(SDL_TRUE);
	Uint32 curTime = SDL_GetTicks();
	Uint32 prevTime = curTime - 1;
	float delta = ((float)curTime - (float)prevTime) / 1000.0f;
	while (!quit) {
		curTime = SDL_GetTicks();
		delta = std::min(((float)curTime - (float)prevTime) / 1000.0f, 0.02f);
		if (delta <= 0) {
			continue;//If there is no ms difference, don't run the loop multiple times
		}
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
			if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.scancode) {
					case SDL_SCANCODE_ESCAPE:
					case SDL_SCANCODE_Q:
						quit = true;
						break;
					case SDL_SCANCODE_P:
					case SDL_SCANCODE_F:
					case SDL_SCANCODE_SPACE:
						gamePaused = !gamePaused;
						break;
					case SDL_SCANCODE_R:
						resetGame();
						break;
					default:
						break;
				}
			}
			if (e.type == SDL_MOUSEMOTION && gamePaused == false) {
				mouseMotion = e.motion.xrel;
				paddleX = std::fmax(std::fmin(1.0 - paddleWidth / 2.0 - (1.0 / ((float)bricksWidth + (float)widthPadding)), paddleX + mouseMotion * delta),
									paddleWidth / 2.0 + (1.0 / ((float)bricksWidth + (float)widthPadding)));
			}
		}
		if (gamePaused == true) {
			continue;
		}
		if (scoreQueueTimer > 0) {
			scoreQueueTimer -= delta;
		}
		if (ballMissed) {
			ballRespawnTimer += delta;
			//Respawn the ball after ballRespawnTime
			if (ballRespawnTimer >= ballRespawnTime) {
				ballMissed = false;
				ballSpeed = ballSpeedStart;
				ballSpeedX = 0;
				ballSpeedY = ballSpeedStart / 2.0f;
				ballX = paddleX;
				ballY = 0.7f;
			}
		} else if (ballY > 1.0f && lives > 0) {
			//Respawn the ball if there are enough lives remaining
			ballMissed = --lives > 0;
			ballSpeedX = 0;
			ballSpeedY = 0;
			ballRespawnTimer = 0;
		}
		//Render scene
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		SDL_RenderClear(ren);
		//Move the ball, handle collisions, and calculate direction and speed
		//Brick collisions
		bool hasCollided = false;
		if ((curTime - lastBreakTime) / 1000.0f > ballCooldown) {
			for (int i = 0; i < bricksWidth; i++) {
				if (hasCollided) {
					break;
				}
				for (int j = 0; j < bricksHeight - 7; j++) {
					if (hasCollided) {
						break;
					}
					if (brickStates[i][j] == 1) {
						float distX = abs(ballX - ((i + 1.5f) / (bricksWidth + (float)widthPadding)));
						float distY = abs(ballY - ((j + 1.5f) / (bricksHeight + (float)heightPadding)));
						//TODO: Add values for the top padding, which will be used above and below to determine where the bricks start vertically
						if (distX <= (ballSize + 1.0f / (bricksWidth + (float)widthPadding)) / 2.0f &&
							distY <= (ballSize + 1.0f / (bricksHeight + (float)heightPadding)) / 2.0f &&
							(ballSpeedY / abs(ballSpeedY)) == -(ballY - ((j + 1.5f) / (bricksHeight + (float)heightPadding))) / abs((ballY - ((j + 1.5f) / (bricksHeight + (float)heightPadding))))) {
							brickStates[i][j] = 0;
							if (j < 4) {
								scoreQueue += 7;
							} else if (j < 8) {
								scoreQueue += 5;
							} else if (j < 12) {
								scoreQueue += 3;
							} else {
								scoreQueue += 1;
							}
							if (j < 4 && ballSpeed < 1.3f * std::min(std::max(1.0f, 1.0f + (score - 1000) / 2000.0f), BALL_SPEED_MULT_CAP)) {
								float oldSpeed = ballSpeed;
								ballSpeed = 1.3f * std::min(std::max(1.0f, 1.0f + (score - 1000) / 2000.0f), BALL_SPEED_MULT_CAP);
								ballSpeedY *= -ballSpeed / oldSpeed;
								ballSpeedX *= ballSpeed / oldSpeed;
							} else if (j < 8 && ballSpeed < 1.1f * std::min(std::max(1.0f, 1.0f + (score - 1000) / 2000.0f), BALL_SPEED_MULT_CAP)) {
								float oldSpeed = ballSpeed;
								ballSpeed = 1.1f * std::min(std::max(1.0f, 1.0f + (score - 1000) / 2000.0f), BALL_SPEED_MULT_CAP);
								ballSpeedY *= -ballSpeed / oldSpeed;
								ballSpeedX *= ballSpeed / oldSpeed;
							} else if (j < 16 && ballSpeed < 0.8f * std::min(std::max(1.0f, 1.0f + (score - 1000) / 2000.0f), BALL_SPEED_MULT_CAP)) {
								float oldSpeed = ballSpeed;
								ballSpeed = 0.8f * std::min(std::max(1.0f, 1.0f + (score - 1000) / 2000.0f), BALL_SPEED_MULT_CAP);
								ballSpeedY *= -ballSpeed / oldSpeed;
								ballSpeedX *= ballSpeed / oldSpeed;
							} else {
								ballSpeedY *= -1.0f;
							}
							lastBreakTime = curTime;
							hasCollided = true;//Only break one brick at a time
							break;
						}
					}
				}
			}
		}
		//Count up the score after hitting bricks
		if (scoreQueue > 0 && scoreQueueTimer <= 0) {
			switch (brickSoundCounter) {
				case 0:
					Mix_PlayChannel(-1, brickSound, 0);
					break;
				case 1:
					Mix_PlayChannel(-1, brickSound2, 0);
					break;
				case 2:
					Mix_PlayChannel(-1, brickSound3, 0);
					break;
				case 3:
					Mix_PlayChannel(-1, brickSound, 0);
					break;
				case 4:
					Mix_PlayChannel(-1, brickSound3, 0);
					break;
				case 5:
					Mix_PlayChannel(-1, brickSound4, 0);
					break;
				case 6:
					Mix_PlayChannel(-1, brickSound, 0);
					break;
				case 7:
					Mix_PlayChannel(-1, brickSound3, 0);
					break;
			}
			brickSoundCounter = (brickSoundCounter + 1) % numBrickSounds;
			score++;
			scoreQueue--;
			scoreQueueTimer = scoreQueueInterval * (1.0f - (lives <= 0) * 0.4f);//Count up the score faster if all lives have been lost
		}
		//Paddle collisions
		if (abs(ballX - paddleX) <= (paddleWidth + ballSize) / 2.0f &&
			abs(ballY - ((bricksHeight + (float)widthPadding) / (bricksHeight + (float)heightPadding) + paddleHeight / 2.0f)) <= ballSize) {
			if ((ballX - paddleX) / abs(ballX - paddleX) < 0) {
				ballSpeedX = -std::max(std::min(abs(ballX - paddleX) / paddleWidth, 0.8f), 0.3f) * ballSpeed;
			} else {
				ballSpeedX = std::max(std::min(abs(ballX - paddleX) / paddleWidth, 0.8f), 0.3f) * ballSpeed;
			}
			ballSpeedY = -abs(ballSpeed - abs(ballSpeedX));
			if ((curTime - lastPaddleHitTime) / 1000.0f > lineCooldown) {
				paddleHitCounter++;
				Mix_PlayChannel(-1, boopSound, 0);
				lastPaddleHitTime = curTime;
			}
			//Move the bricks down
			if ((curTime - lastLineTime) / 1000.0f > lineCooldown &&
				(paddleHitCounter == 9 || paddleHitCounter == 14 || paddleHitCounter == 17 || paddleHitCounter == 20 || paddleHitCounter == 22 ||
				 paddleHitCounter == 24 || (paddleHitCounter > 25 && paddleHitCounter % 2 == 0))) {
				Mix_PlayChannel(-1, lineSound, 0);
				for (int j = bricksHeight - 8; j >= 0; j--) {
					for (int i = bricksWidth - 1; i >= 0; i--) {
						if (j > 0) {
							brickStates[i][j] = brickStates[i][j - 1];
						} else if (j == 0) {
							brickStates[i][j] = lineCounter % 8 > 3;
						}
					}
				}
				lineCounter++;
				lastLineTime = curTime;
				//Shrink the paddle
				if (lineCounter > 99 && paddleWidth > paddleWidthStart * 0.85f) {
					paddleWidth = paddleWidthStart * 0.85f;
				} else if (lineCounter > 199 && paddleWidth > paddleWidthStart * 0.75f) {
					paddleWidth = paddleWidthStart * 0.75f;
				} else if (lineCounter > 299 && paddleWidth > paddleWidthStart * 0.5f) {
					paddleWidth = paddleWidthStart * 0.5f;
				}
			}
		}
		//Wall collisions
		if (ballX <= 1.0f / (bricksWidth + (float)widthPadding) + ballSize / 2.0f) {
			ballSpeedX = abs(ballSpeedX);
			Mix_PlayChannel(-1, boopSound, 0);
		} else if (ballX >= (bricksWidth + (float)widthPadding - 1.0) / (bricksWidth + (float)widthPadding) - ballSize / 2.0f) {
			ballSpeedX = -abs(ballSpeedX);
			Mix_PlayChannel(-1, boopSound, 0);
		}
		if (ballY <= 1.0f / (bricksHeight + (float)heightPadding) + ballSize / 2.0f) {
			ballSpeedY = abs(ballSpeedY);
			Mix_PlayChannel(-1, boopSound, 0);
		}
		//After calculating collisions, move the ball
		ballX += ballSpeedX * delta;
		ballY += ballSpeedY * delta;
		//Draw bricks
		for (int i = 0; i < bricksWidth + 2; i++) {
			for (int j = 0; j < bricksHeight + 8; j++) {
				//Fill border blocks
				if (i == 0 || i == bricksWidth + 1 || j == 0) {
					SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
					SDL_Rect r;
					r.x = ceil((i / (bricksWidth + (float)widthPadding)) * WIDTH);
					r.y = ceil((scoreMargin + (j / (bricksHeight + (float)heightPadding))) * HEIGHT);
					r.w = ceil((1.0 / ((float)bricksWidth + (float)widthPadding)) * WIDTH);
					r.h = ceil((1.0 / ((float)bricksHeight + (float)heightPadding)) * HEIGHT);
					SDL_RenderFillRect(ren, &r);
				} else if (j < bricksHeight && brickStates[i - 1][j - 1] == 1) {
					if (j < 5) {
						SDL_SetRenderDrawColor(ren, 220 - ((j - 4) * -20), 0, 0, 255);
					} else if (j < 9) {
						SDL_SetRenderDrawColor(ren, 0, 0, 230 - ((j - 8) * -20), 255);
					} else if (j < 13) {
						SDL_SetRenderDrawColor(ren, 200 - ((j - 12) * -20), 0, 140 - ((j - 12) * -20), 255);
					} else if (j < 17) {
						SDL_SetRenderDrawColor(ren, 0, 220 - ((j - 16) * -20), 0, 255);
					} else {
						SDL_SetRenderDrawColor(ren, 0, 220 - ((j - 20) * -20), 0, 255);
					}
					SDL_Rect r;
					r.x = floor(-0.005 + ((i) / ((float)bricksWidth + (float)widthPadding)) * WIDTH);
					r.y = ceil(0.05 + (scoreMargin + (j / ((float)bricksHeight + (float)heightPadding))) * HEIGHT);
					r.w = floor((1.01 / ((float)bricksWidth + (float)widthPadding)) * WIDTH);
					r.h = ceil((0.9 / ((float)bricksHeight + (float)heightPadding)) * HEIGHT);
					SDL_RenderFillRect(ren, &r);
				}
			}
		}
		//Draw the paddle and ball
		SDL_SetRenderDrawColor(ren, 150, 170, 0, 255);
		SDL_Rect p;
		p.x = (paddleX - paddleWidth / 2.0) * WIDTH;
		p.y = (scoreMargin + (bricksHeight + (float)widthPadding) / (bricksHeight + (float)heightPadding)) * HEIGHT;
		p.w = paddleWidth * WIDTH;
		p.h = (1.0 / ((float)bricksHeight + (float)heightPadding)) * HEIGHT;
		SDL_RenderFillRect(ren, &p);
		SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
		SDL_Rect b;
		b.x = (ballX - ballSize / 2.0) * WIDTH;
		b.y = (scoreMargin + (ballY - ballSize / 2.0)) * HEIGHT;
		b.w = ballSize * WIDTH;
		b.h = ballSize * HEIGHT;
		SDL_RenderFillRect(ren, &b);
		//Draw the score and lives
		for (int i = 0; i < SCORE_DIGITS; i++) {
			renderTexture(digitImg[(int)(score / std::powf(10, SCORE_DIGITS - 1 - i)) % 10], ren, i * scoreMargin * 0.9f * WIDTH, 0.05f * scoreMargin * HEIGHT, scoreMargin * 0.9f * 0.8f * WIDTH, scoreMargin * 0.9f * HEIGHT);
		}
		renderTexture(digitImg[std::max(lives, 0)], ren, (SCORE_DIGITS + 3) * scoreMargin * 0.9f * WIDTH, 0.05f * scoreMargin * HEIGHT, scoreMargin * 0.9f * 0.8f * WIDTH, scoreMargin * 0.9f * HEIGHT);
		SDL_RenderPresent(ren);
		prevTime = curTime;
	}
	for (int i = 0; i < 10; i++) {
		cleanup(digitImg[i]);
	}
	cleanup(ren, window);
	Mix_FreeChunk(boopSound);
	Mix_FreeChunk(brickSound);
	Mix_FreeChunk(lineSound);
	Mix_Quit();
	SDL_Quit();
	return EXIT_SUCCESS;
}