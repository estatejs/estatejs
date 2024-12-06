class Player extends Atom {
    constructor() {
        super({
            id: 1,
            shared: false
        });
        this.username = null;
    }
    set name(name_value) {
        this.username = name_value;
    }
    get current_name() {
        return this.username;
    }
    reset_name_to(reset_name_to_value) {
        this.username = reset_name_to_value;
    }
}

class Game extends Atom {
    constructor() {
        super({
            id: 2,
            shared: true
        });
        this.name = null;
    }
    set game_name(game_name_value) {
        this.name = game_name_value;
    }
    get current_game_name() {
        return this.name;
    }
    reset_game_name_to(reset_game_name_to_value) {
        this.name = reset_game_name_to_value;
    }
}