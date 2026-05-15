NAME :=	taskmaster

CXX :=	c++
CXXFLAGS :=	-g -MP -MMD # -Wall -Wextra -Werror -std=c++17 # -fsanitize=address -fno-omit-frame-pointer
LFLAGS :=

###

INCLUDE_DIRS :=	inc/	\

SRCS :=	src/main.cpp	\
		src/Process.cpp	\

###

INCLUDE_DIRS :=	$(addprefix -I, $(INCLUDE_DIRS))

###

OBJ_DIR :=	obj

OBJS =	$(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS =	$(SRCS:%.cpp=$(OBJ_DIR)/%.d)

###

compile:
	@make -j all --no-print-directory

all: $(NAME)

$(NAME): $(OBJS)
	@echo Compiling $(NAME)
	@$(CXX) $(CXXFLAGS) $(LFLAGS) $(INCLUDE_DIRS) -o $@ $^

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo Compiling $@
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

re: fclean compile

fclean: clean
	@echo Removed $(NAME)
	@rm -rf $(NAME)

clean:
	@echo Removed $(OBJ_DIR)
	@rm -rf $(OBJ_DIR)

.PHONY: all clean fclean re compile

-include $(DEPS)
