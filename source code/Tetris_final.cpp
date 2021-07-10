#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <SDL_image.h>
#include <SDL.h>
#include <SDL_ttf.h>
#undef main

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;
typedef double f64;

#include "colors.h"

#define WIDTH 10
#define HEIGHT 22
#define VISIBLE_HEIGHT 20
#define GRID_SIZE 30

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

const u8 FRAMES_PER_DROP[] = {
    48,43,38,33,28,
    23,
    18,
    13,
    8,
    6,
    5,
    5,
    5,
    4,
    4,
    4,
    3,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    1
};

const f32 TARGET_SECONDS_PER_FRAME = 1.f / 60.f;

struct Tetrino
{
    const u8* data;
    const s32 side;
};

inline Tetrino
tetrino(const u8* data, s32 side)
{
    return { data, side }; // trả về struct: chứa con trỏ đến khối và số cạnh
}

const u8 TETRINO_1[] = {
    0, 0, 0, 0,
    1, 1, 1, 1,
    0, 0, 0, 0,
    0, 0, 0, 0
};

const u8 TETRINO_2[] = {
    2, 2,
    2, 2
};

const u8 TETRINO_3[] = {
    0, 0, 0,
    3, 3, 3,
    0, 3, 0
};

const u8 TETRINO_4[] = {
    0, 4, 4,
    4, 4, 0,
    0, 0, 0
};

const u8 TETRINO_5[] = {
    5, 5, 0,
    0, 5, 5,
    0, 0, 0
};

const u8 TETRINO_6[] = {
    6, 0, 0,
    6, 6, 6,
    0, 0, 0
};

const u8 TETRINO_7[] = {
    0, 0, 7,
    7, 7, 7,
    0, 0, 0
};


const Tetrino TETRINOS[] = {  // mảng chứa các struct đủ thông tin(data: là con trỏ trỏ đến khối, side là ô chứa)
    tetrino(TETRINO_1, 4),
    tetrino(TETRINO_2, 2),
    tetrino(TETRINO_3, 3),
    tetrino(TETRINO_4, 3),
    tetrino(TETRINO_5, 3),
    tetrino(TETRINO_6, 3),
    tetrino(TETRINO_7, 3),
};

enum Game_Phase    // khai báo 1 enum chứa các phase game
{
    GAME_PHASE_START,
    GAME_PHASE_PLAY,
    GAME_PHASE_LINE,
    GAME_PHASE_GAMEOVER,
    GAME_PHASE_PAUSE
};

inline s32 random_int(s32 min, s32 max)
{
    s32 range = max - min;
    return min + rand() % range;
}

u8 next_piece = (u8)random_int(0, ARRAY_COUNT(TETRINOS));

struct Piece_State     //trạng thái của 1 khối:  khối nào, vị trí đã offset,  rotation: dã quay mấy lần
{
    u8 tetrino_index;
    s32 offset_row;
    s32 offset_col;
    s32 rotation;
};

struct Game_State         //WIDTH, HEIGHT là chiều cao và chiều rộng.
{
    u8 board[WIDTH * HEIGHT];   // board game là 1 mảng 1 chiều, có số phần tử là số ô, 
    u8 lines[HEIGHT];
    s32 pending_line_count;  // hàng có ô đầy đợi xóa

    Piece_State piece;

    Game_Phase phase;

    s32 start_level;  //?????
    s32 level;  //level hiện tại
    s32 line_count; // số dòng đã ăn điểm
    s32 points;  //điểm

    f32 next_drop_time;    // xđ lần rơi tiếp theo, bằng thời gian rơi lần trc + kc giữa 2 lần rơi
    f32 highlight_end_time; // xđ thời gian hightlight, hết hightlight game chạy tiêp
    f32 time; //cập nhật liên tục, là thời gian chương trình chạy
};

struct Input_State
{
    u8 left;
    u8 right;
    u8 up;
    u8 down;

    u8 a;

    u8 pause;

    s8 dleft;               //d-x > 0 tương ứng với việc có thao tác 
    s8 dright;
    s8 dup;
    s8 ddown;
    s8 da;
    s8 dpause;
};

enum Text_Align
{
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT
};

inline u8 matrix_get(const u8* values, s32 width, s32 row, s32 col) //trả về giá trị của phần tử trong mảng
{ //trả về 1 mảng 1 chiều có số phần tử đc cung cấp
    s32 index = row * width + col;
    return values[index];
}

inline void matrix_set(u8* values, s32 width, s32 row, s32 col, u8 value) // gán giá trị trong mảng bằng bn đ
{
    s32 index = row * width + col;
    values[index] = value;
}

inline u8 tetrino_get(const Tetrino* tetrino, s32 row, s32 col, s32 rotation) 
// lấy ra giá trị 1,2,3... tùy theo khối, tùy theo số lấn xoay
// lấy giá trị ở các ô chứa giá trị để mô tả khối
{ 
    s32 side = tetrino->side; // lấy số cạnh
    switch (rotation)
    {
    case 0:
        return tetrino->data[row * side + col];
    case 1:
        return tetrino->data[(side - col - 1) * side + row];
    case 2:
        return tetrino->data[(side - row - 1) * side + (side - col - 1)];
    case 3:
        return tetrino->data[col * side + (side - row - 1)];
    }
    return 0;
}

inline u8 check_row_filled(const u8* values, s32 width, s32 row)
{
    for (s32 col = 0;
        col < width;
        ++col)
    {
        if (!matrix_get(values, width, row, col))   // ko get đc ==> chưa đầy
        {
            return 0;
        }
    }
    return 1;
}

inline u8
check_row_empty(const u8* values, s32 width, s32 row)
{
    for (s32 col = 0;
        col < width;
        ++col)
    {
        if (matrix_get(values, width, row, col))  //vẫn get đc ==> ko rỗng
        {
            return 0;
        }
    }
    return 1;
}

s32 find_lines(const u8* values, s32 width, s32 height, u8* lines_out)
{   //trả về số dòng đầy thôi, dòng nào đầy đc set = 1
    s32 count = 0;
    for (s32 row = 0; row < height; ++row)
    {
        u8 filled = check_row_filled(values, width, row);
        lines_out[row] = filled;  // đếm số dòng đã đã đầy,
        count += filled;
    }
    return count;  //trả về dòng đã đầy
}

void clear_lines(u8* values, s32 width, s32 height, const u8* lines)
{
    s32 src_row = height - 1;
    for (s32 dst_row = height - 1; dst_row >= 0; --dst_row)
    {
        while (src_row >= 0 && lines[src_row])        // chưa đến dòng cuối và chưa gặp dòng trống thì tiếp tục lặp
        {
            --src_row;
        }

        if (src_row < 0)
        {
            memset(values + dst_row * width, 0, width); //values là mảng chứa 1 dòng
        }
        else
        {
            if (src_row != dst_row)
            {
                memcpy(values + dst_row * width, values + src_row * width,width);
            }
            --src_row;
        }
    }
}


bool check_piece_valid(const Piece_State* piece,const u8* board, s32 width, s32 height)
{   // check xem có chạy đc nữa hay ko
    const Tetrino* tetrino = TETRINOS + piece->tetrino_index;
    assert(tetrino);

    for (s32 row = 0;row < tetrino->side;++row)
    {
        for (s32 col = 0;col < tetrino->side;++col)
        {
            u8 value = tetrino_get(tetrino, row, col, piece->rotation);
            if (value > 0)
            {
                s32 board_row = piece->offset_row + row;
                s32 board_col = piece->offset_col + col;
                if (board_row < 0)  //ko cho chạy lên trên
                {
                    return false;
                }
                if (board_row >= height)  // ko cho chạy xuống dưới
                {
                    return false;
                }
                if (board_col < 0)  //ko cho chạy quá trái
                {
                    return false;
                }
                if (board_col >= width) // ko cho chạy quá phải
                {
                    return false;
                }
                if (matrix_get(board, width, board_row, board_col))  // ô đó đã chứa cell khác
                {
                    return false;
                }
            }
        }
    }
    return true;
}

void merge_piece(Game_State* game)    //các ô có cell trong board sẽ chứa giá trị
{
    const Tetrino* tetrino = TETRINOS + game->piece.tetrino_index;
    for (s32 row = 0;row < tetrino->side;++row)
    {
        for (s32 col = 0;col < tetrino->side;++col)
        {
            u8 value = tetrino_get(tetrino, row, col, game->piece.rotation);
            if (value)
            {
                s32 board_row = game->piece.offset_row + row;   // đi xuống bn row
                s32 board_col = game->piece.offset_col + col;   // đi sang bn col
                matrix_set(game->board, WIDTH, board_row, board_col, value); //set giá trị vào board. ==> các ô đã chứa cell đc set
            }
        }
    }
}


inline f32 get_time_to_next_drop(s32 level)
{
    if (level > 29)
    {
        level = 29;
    }
    return FRAMES_PER_DROP[level] * TARGET_SECONDS_PER_FRAME;   //xác định mỗi drop là bn phần của giây, tương ứng với bn frame trên giây
}


void spawn_piece(Game_State* game,int number)    // sinh ra các khối tetris
{ //set up giá trị cho game state: khối nào, vị trí ở giữa board, khoảng thời gian giữa 2 lần rơi
    
    game->piece = {};
    game->piece.tetrino_index = number;  // sinh ra 1 khối bất kỳ vs các thuộc tính xác định
    game->piece.offset_col = WIDTH / 2;
    game->next_drop_time = game->time + get_time_to_next_drop(game->level);
}


inline bool soft_drop(Game_State* game)
{   // hiệu ứng rơi xuống, tăng row lên
    ++game->piece.offset_row;
    if (!check_piece_valid(&game->piece, game->board, WIDTH, HEIGHT)) //check xem có movable hay ko
    {
        --game->piece.offset_row;       // ko move đc: dòng là dòng trc đó
        merge_piece(game);              // set giá trị vào board
        spawn_piece(game,next_piece);              // sinh khối mới
        next_piece = (u8)random_int(0, ARRAY_COUNT(TETRINOS));
        return false;
    }

    game->next_drop_time = game->time +  get_time_to_next_drop(game->level);     //tiếp tục rơi, xđ thời gian rơi lần tiếp theo
    return true;
}

inline s32 compute_points(s32 level, s32 line_count)
{
    switch (line_count)                 //tính điểm, ăn 1 dòng đc 40 * (level+1) mà bạn đang ở. 
    {
    case 1:
        return 40 * (level + 1);
    case 2:
        return 100 * (level + 1);
    case 3:
        return 300 * (level + 1);
    case 4:
        return 1200 * (level + 1);
    }
    return 0;
}

inline s32 min(s32 x, s32 y)
{
    return x < y ? x : y;
}
inline s32 max(s32 x, s32 y)
{
    return x > y ? x : y;
}

inline s32 get_lines_for_next_level(s32 start_level, s32 level)
{
    s32 first_level_up_limit = min((start_level * 10 + 10), max(100, (start_level * 10 - 50)));   // tùy thuộc vào start level mà sẽ có cách thức tính tăng level khác nhau
    if (level == start_level)  //khi ở level=0 thì cần 
    {
        return first_level_up_limit;
    }
    s32 diff = level - start_level;
    return first_level_up_limit + diff * 10;
}

void update_game_start(Game_State* game, const Input_State* input)
{
    if (input->dup > 0) //bấm up/down để tăng level
    {
        ++game->start_level;
    }

    if (input->ddown > 0 && game->start_level > 0)
    {
        --game->start_level;
    }

    if (input->da > 0)  
    {
        memset(game->board, 0, WIDTH * HEIGHT);  //set cả mảng board thành 0, ==> chưa chứa gì
        game->level = game->start_level;
        game->line_count = 0;
        game->points = 0;
        spawn_piece(game,next_piece);
        game->phase = GAME_PHASE_PLAY;
    }
}

void update_game_gameover(Game_State* game, const Input_State* input)
{   
    high_score = (game->points > high_score)?(game->points):(high_score);
    if (input->da > 0)
    {
        game->phase = GAME_PHASE_START;
    }
}

void update_game_line(Game_State* game)
{
    if (game->time >= game->highlight_end_time) // time: thời gian đã chạy chương trình
    {
        clear_lines(game->board, WIDTH, HEIGHT, game->lines);
        game->line_count += game->pending_line_count;  //số dòng ăn điểm= bị xóa
        game->points += compute_points(game->level, game->pending_line_count);      // điểm đc tính dựa vào level và số dòng ăn điểm

        s32 lines_for_next_level = get_lines_for_next_level(game->start_level, game->level);    //đạt đủ số line thì lên level
        if (game->line_count >= lines_for_next_level)
        {
            ++game->level;
        }

        high_score = (game->points > high_score) ? (game->points) : (high_score);
        game->phase = GAME_PHASE_PLAY;
    }
}

void update_game_play(Game_State* game, const Input_State* input)
{
    Piece_State piece = game->piece;
    if (input->dleft >= 1)    //sang trái
    {
        --piece.offset_col;
    } 

    if (input->dright > 0)  //sang phải
    {
        ++piece.offset_col;
    }

    if (input->dup > 0) //quay
    {
        piece.rotation = (piece.rotation + 1) % 4;
    }

    if (check_piece_valid(&piece, game->board, WIDTH, HEIGHT))
    {
        game->piece = piece;
    }

    if (input->ddown > 0)   //bấm down ==> xuống 
    {
        soft_drop(game);
    }

    if (input->da > 0)   //bấm space: xuống đến khi nào ko xuống đc, xuống nhanh
    {
        while (soft_drop(game));
    }

    while (game->time >= game->next_drop_time) // đi xuống 1 cách bthg
    {
        soft_drop(game);
    }

    game->pending_line_count = find_lines(game->board, WIDTH, HEIGHT, game->lines); //tìm hàng đầy

    if (game->pending_line_count > 0)   // số hàng đầy lớn hơn ko
    {
        game->phase = GAME_PHASE_LINE;
        game->highlight_end_time = game->time + 0.5f;
    }

    s32 game_over_row = 0;
    if (!check_row_empty(game->board, WIDTH, game_over_row))      //hàng số 0 mà có cell thì over
    {
        game->phase = GAME_PHASE_GAMEOVER;
    }

    if (input->dpause > 0) {
        game->phase = GAME_PHASE_PAUSE;
    }

}

void update_game_pause(Game_State* game, const Input_State* input) 
{
    if (input->da > 0) //nhấn SPACE để tiếp tục trò chơi
    {
        game->phase = GAME_PHASE_PLAY;
    }
}

void update_game(Game_State* game, const Input_State* input)
{
    switch (game->phase)
    {
    case GAME_PHASE_START: //bắt đầu game
        update_game_start(game, input);
        break;
    case GAME_PHASE_PLAY: //đang chơi
        update_game_play(game, input);
        break;
    case GAME_PHASE_LINE: //ăn điểm
        update_game_line(game);
        break;
    case GAME_PHASE_GAMEOVER: //thua
        update_game_gameover(game, input);
        break;
    case GAME_PHASE_PAUSE:
        update_game_pause(game, input);
    }
}

void fill_rect(SDL_Renderer* renderer, s32 x, s32 y, s32 width, s32 height, Color color)
{   //vẽ ra 1 hcn đặc
    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect); // vẽ ra 1 hcn có màu cho trước [hình chạy]
}


void draw_rect(SDL_Renderer* renderer, s32 x, s32 y, s32 width, s32 height, Color color)
{   //vẽ ra 1 hcn rỗng
    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rect); //vẽ 1 hcn on the current rendering target[hình ở phía dưới]
}

void draw_string(SDL_Renderer* renderer, TTF_Font* font, const char* text, s32 x, s32 y, Text_Align alignment, Color color)
{
    uint8_t r1 = color.r;
    uint8_t r2 = color.g;
    uint8_t r3 = color.b;
    uint8_t r4 = color.a;
    SDL_Color sdl_color = SDL_Color{ r1, r2, r3, r4 };
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, sdl_color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect rect;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;
    switch (alignment)
    {
    case TEXT_ALIGN_LEFT:
        rect.x = x;
        break;
    case TEXT_ALIGN_CENTER:
        rect.x = x - surface->w / 2;
        break;
    case TEXT_ALIGN_RIGHT:
        rect.x = x - surface->w;
        break;
    }

    SDL_RenderCopy(renderer, texture, 0, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}


void draw_string_shaded(SDL_Renderer* renderer, TTF_Font* font, const char* text, s32 x, s32 y, Text_Align alignment, Color color)
{
    uint8_t r1 = color.r;
    uint8_t r2 = color.g;
    uint8_t r3 = color.b;
    uint8_t r4 = color.a;
    SDL_Color sdl_color = SDL_Color{ r1, r2, r3, r4 };
    SDL_Color bgcolor = { 0xff,0xff,0xff };
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, sdl_color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect rect;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;
    switch (alignment)
    {
    case TEXT_ALIGN_LEFT:
        rect.x = x;
        break;
    case TEXT_ALIGN_CENTER:
        rect.x = x - surface->w / 2;
        break;
    case TEXT_ALIGN_RIGHT:
        rect.x = x - surface->w;
        break;
    }

    SDL_RenderCopy(renderer, texture, 0, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}


void draw_cell(SDL_Renderer* renderer, s32 row, s32 col, u8 value, s32 offset_x, s32 offset_y, bool outline = false)
{  //vẽ ra các ô nhỏ để hợp thành khối
    Color base_color = BASE_COLORS[value];
    Color light_color = LIGHT_COLORS[value];
    Color dark_color = DARK_COLORS[value];

    s32 edge = GRID_SIZE / 8; //xác định độ to nhỏ viền của cell 

    s32 x = col * GRID_SIZE + offset_x;   // cách từ trái sang bn
    s32 y = row * GRID_SIZE + offset_y;  // cách từ phải sang bn đấy

    if (outline)        // có outline ==> hình phía dưới, vẽ khác
        //dùng cho khi vẽ các khối biểu trưng ở phía dưới
    {
        draw_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, base_color);
        return;
    }

    // mỗi 1 ô vuông nhỏ có kích thước 30x30px và có viền. 
    fill_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, dark_color);
    fill_rect(renderer, x + edge, y, GRID_SIZE - edge, GRID_SIZE - edge, light_color);
    fill_rect(renderer, x + edge, y + edge, GRID_SIZE - edge * 2, GRID_SIZE - edge * 2, base_color);
}

void draw_piece(SDL_Renderer* renderer, const Piece_State* piece, s32 offset_x, s32 offset_y, bool outline = false)
{       // sau khi vẽ đc 1 ô thì gộp lại để đc 1 khối
    const Tetrino* tetrino = TETRINOS + piece->tetrino_index;  // lấy ra đc 1 khối
    for (s32 row = 0; row < tetrino->side; ++row)
    {
        for (s32 col = 0; col < tetrino->side; ++col)
        {
            u8 value = tetrino_get(tetrino, row, col, piece->rotation); // tùy theo số lần quay mà cách lấy các ô vị trí khác nhau
            if (value)
            {
                draw_cell(renderer, row + piece->offset_row, col + piece->offset_col, value, offset_x, offset_y, outline);
            }
        }
    }
}

void draw_board(SDL_Renderer* renderer, const u8* board, s32 width, s32 height, s32 offset_x, s32 offset_y) // khu vực chơi
{
    fill_rect(renderer, offset_x, offset_y, width * GRID_SIZE, height * GRID_SIZE, LIGHT_COLORS[0]);
    
    for (s32 row = 0; row < height; ++row)
    {
        for (s32 col = 0; col < width; ++col)
        {
            u8 value = matrix_get(board, width, row, col);   // duyệt qua khung trò chơi, ở đâu có số thì vẽ vào hình thích hợp
            if (value)
            {
                draw_cell(renderer, row, col, value, offset_x, offset_y);
            }
        }
    }
}

void draw_background(SDL_Renderer* renderer) {
    const char* image = "START-back.png";
    SDL_Surface* load_image = IMG_Load(image);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, load_image);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
}

void render_game(const Game_State* game, SDL_Renderer* renderer, TTF_Font* font)
{
    char buffer[4096];

    Color highlight_color = color(0xFF, 0xFF, 0x00, 0x00);

    s32 margin_y = 60;

    draw_board(renderer, game->board, WIDTH, HEIGHT, 5, 0); // board game rộng thực 300x660px, rộng ảo 300x600px

    Piece_State next_tetris;
    next_tetris.tetrino_index = next_piece;
    next_tetris.rotation = 0;

    if (game->phase == GAME_PHASE_PLAY)
    {
        draw_piece(renderer, &game->piece, 5, 0);

        Piece_State piece = game->piece;

        while (check_piece_valid(&piece, game->board, WIDTH, HEIGHT))
        {
            piece.offset_row++;
        }
        --piece.offset_row;

        draw_piece(renderer, &piece, 5, 0, true);
        draw_piece(renderer, &next_tetris, 350, 480);
    }

    if (game->phase == GAME_PHASE_LINE) // check khi 1 dòng đc lấp đầy
    {
        for (s32 row = 0; row < HEIGHT; ++row)
        {
            if (game->lines[row])
            {
                s32 x = 5;
                s32 y = row * GRID_SIZE;

                fill_rect(renderer, x, y,
                    WIDTH * GRID_SIZE, GRID_SIZE, color(0xFF, 0xFF, 0xFF, 0xFF));
            }
        }
        draw_piece(renderer, &next_tetris, 350, 480);
    }
    else if (game->phase == GAME_PHASE_GAMEOVER)
    {
        s32 x = WIDTH * GRID_SIZE / 2;
        s32 y = ((HEIGHT-2) * GRID_SIZE) / 2;
        draw_string(renderer, font, "GAME OVER", x, y, TEXT_ALIGN_CENTER, highlight_color);
    }
    else if (game->phase == GAME_PHASE_START)
    {
        s32 x = WIDTH * GRID_SIZE / 2;
        s32 y = ((HEIGHT-2) * GRID_SIZE) / 2;

        snprintf(buffer, sizeof(buffer), "STARTING LEVEL: %d", game->start_level);
        draw_string_shaded(renderer, font, buffer, x, y, TEXT_ALIGN_CENTER, color(0xFF, 0xFF, 0xFF, 0xFF));

        draw_string(renderer, font, "PRESS SPACE TO START", x, y + 30, TEXT_ALIGN_CENTER, color(0xFF, 0xFF, 0xFF, 0xFF));

        draw_string(renderer, font, "Press Up/Down to select level", x, y+60, TEXT_ALIGN_CENTER, color(0xFF, 0xFF, 0xFF, 0xFF));

        snprintf(buffer, sizeof(buffer), "HIGH SCORE: %d", high_score);
        draw_string_shaded(renderer, font, buffer, x, y+90, TEXT_ALIGN_CENTER, color(0xFF, 0xFF, 0xFF, 0xFF));

    }

    else if (game->phase == GAME_PHASE_PAUSE)
    {
        s32 x = WIDTH * GRID_SIZE / 2;
        s32 y = (HEIGHT * GRID_SIZE) / 2;
        draw_string(renderer, font, "PAUSE", x, y, TEXT_ALIGN_CENTER, highlight_color);
        draw_string(renderer, font, "Press SPACE to continue", x, y + 60, TEXT_ALIGN_CENTER, highlight_color);
    }

    fill_rect(renderer, 5, 0, WIDTH * GRID_SIZE, (HEIGHT - VISIBLE_HEIGHT) * GRID_SIZE, /*color(0xE5, 0x44, 0x44, 0xFF)*/ LIGHT_COLORS[0]);
    fill_rect(renderer, 305, 0, 5, (HEIGHT * GRID_SIZE)+5, color(0xFF, 0xFF, 0x00, 0x00));
    fill_rect(renderer, 305, 130, 150, 5, color(0xFF, 0xFF, 0x00, 0x00));
    fill_rect(renderer, 305, 250, 150, 5, color(0xFF, 0xFF, 0x00, 0x00));
    fill_rect(renderer, 305, 390, 150, 5, color(0xFF, 0xFF, 0x00, 0x00));
    fill_rect(renderer, 0, 60, 305, 5, color(0xFF, 0xFF, 0x00, 0x00));
    fill_rect(renderer, 0, 0, 5, 665, color(0xFF, 0xFF, 0x00, 0x00));
    fill_rect(renderer, 0, 660, 305, 5, color(0xFF, 0xFF, 0x00, 0x00));

    draw_string(renderer, font, "High Scores:", 375, 30, TEXT_ALIGN_CENTER, color(0xFF, 0xFF, 0x00, 0x00));
    snprintf(buffer, sizeof(buffer), "%d", high_score);
    draw_string(renderer, font, buffer, 375, 80, TEXT_ALIGN_CENTER, highlight_color);

    snprintf(buffer, sizeof(buffer), "Level: %d", game->level);
    draw_string(renderer, font, buffer, 50, 20, TEXT_ALIGN_LEFT, color(0xFF, 0xFF, 0x00, 0x00));

    draw_string(renderer, font, "Lines:", 375, 150, TEXT_ALIGN_CENTER, color(0xFF, 0xFF, 0x00, 0x00));
    snprintf(buffer, sizeof(buffer), "%d", game->line_count);
    draw_string(renderer, font, buffer, 375, 200, TEXT_ALIGN_CENTER, highlight_color);

    draw_string(renderer, font, "Scores:", 375, 270, TEXT_ALIGN_CENTER, color(0xFF, 0xFF, 0x00, 0x00));
    snprintf(buffer, sizeof(buffer), "%d", game->points);
    draw_string(renderer, font, buffer, 375, 320, TEXT_ALIGN_CENTER, highlight_color);

    draw_string(renderer, font, "Next: ", 375, 400, TEXT_ALIGN_CENTER, highlight_color);


    draw_string(renderer, font, "Press 'P' to pause", 385, 540, TEXT_ALIGN_CENTER, highlight_color);
    
    
}


int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return 1;

    }
    IMG_Init(IMG_INIT_PNG);

    if (TTF_Init() < 0)
    {
        return 2;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Tetris",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        455,
        665,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    const char* image = "START-back-1.png";
    SDL_Surface* load_image = IMG_Load(image);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, load_image);


    const char* font_name = "Roboto-Bold.ttf";
    TTF_Font* font = TTF_OpenFont(font_name, 16);

    Game_State game = {};
    Input_State input = {};

    //spawn_piece(&game);  //set up piece

    //game.piece.tetrino_index = 5;
   
    bool quit = false;
    while (!quit)
    {
        game.time = SDL_GetTicks() / 500.0f;  //trả về số mgiây từ khi SDL đc khởi tạo

        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
        }

        s32 key_count;

        const u8* key_states = SDL_GetKeyboardState(&key_count);

        if (key_states[SDL_SCANCODE_ESCAPE])
        {
            quit = true;
        }

        Input_State prev_input = input;

        input.left = key_states[SDL_SCANCODE_LEFT];
        
        input.right = key_states[SDL_SCANCODE_RIGHT];
        input.up = key_states[SDL_SCANCODE_UP];
        input.down = key_states[SDL_SCANCODE_DOWN];
        input.a = key_states[SDL_SCANCODE_SPACE];
        input.pause = key_states[SDL_SCANCODE_P];

        input.dleft = input.left - prev_input.left;
        input.dright = input.right - prev_input.right;
        input.dup = input.up - prev_input.up;
        input.ddown = input.down - prev_input.down;
        input.da = input.a - prev_input.a;
        input.dpause = input.pause - prev_input.pause;

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        
        update_game(&game, &input);

        render_game(&game, renderer, font);
        SDL_RenderPresent(renderer);
    }

    IMG_Quit();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    return 0;
}
