class Player extends Data {
    constructor() {
        super();
        this.username = "scott";
    }
    set name(name_value) {
        this.username = name_value;
    }
    get current_name() {
        return this.username;
    }
    reset_name_to(reset_name_to_value) {
        this.username = reset_name_to_value;
        return {
            new_value: reset_name_to_value
        };
    }
}
