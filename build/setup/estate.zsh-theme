# Depends on the git plugin for work_in_progress()
FG_LIGHTGREEN="$FG[113]"
FG_DEEPRED="$FG[196]"
FG_ORANGE="$FG[208]"
FG_LIGHTORANGE="$FG[172]"
FG_BLUE="$FG[033]"
FG_LIGHTBLUE="$FG[004]"

if [ ! -z "${SSH_TTY}" ]; then
    #Remote
    host_color="%{$FG_ORANGE%}"
    arm_color="%{$FG_LIGHTORANGE%}"
else
    #Local
    host_color="%{$FG_BLUE%}"
    arm_color="%{$FG_LIGHTBLUE%}"
fi

ARM_DOWN="%{$reset_color%}${arm_color}╭╼%{$reset_color%}"
ARM_UP="%{$reset_color%}${arm_color}╰╼%{$reset_color%}"

host_prefix="${host_color}$(hostname | sed 's/[^ ]\+/\L\u&/g')%{$reset_color%}"

ZSH_THEME_GIT_PROMPT_PREFIX="%{$reset_color%}%{$fg[green]%}["
ZSH_THEME_GIT_PROMPT_SUFFIX="]%{$reset_color%}"
ZSH_THEME_GIT_PROMPT_DIRTY="%{$fg[red]%}*%{$reset_color%}"
ZSH_THEME_GIT_PROMPT_CLEAN=""

#Customized git status, oh-my-zsh currently does not allow render dirty status before branch
function git_custom_status() {
  local cb=$(git_current_branch)
  if [ -n "$cb" ]; then
    echo "$(parse_git_dirty)%{$fg_bold[yellow]%}$(work_in_progress)%{$reset_color%}$ZSH_THEME_GIT_PROMPT_PREFIX$(git_current_branch)$ZSH_THEME_GIT_PROMPT_SUFFIX"
  fi
}

# RVM component of prompt
ZSH_THEME_RUBY_PROMPT_PREFIX="%{$fg[red]%}["
ZSH_THEME_RUBY_PROMPT_SUFFIX="]%{$reset_color%}"

# Combine it all into a final right-side prompt
RPS1='$(git_custom_status)$(ruby_prompt_info) $EPS1'


if [[ $EUID -eq 0 ]]; then
    user_symbol="#"
else
    user_symbol="$"
fi

prompt_symbol_ok="${reset_color}${FG_LIGHTGREEN}${user_symbol}"
prompt_symbol_err="${reset_color}${FG_DEEPRED}${user_symbol}"


PROMPT="${ARM_DOWN}${host_prefix}%{$fg[cyan]%}[%~% ]
${ARM_UP}%B%(?.%{$prompt_symbol_ok%}.%{$prompt_symbol_err%})%b  "
