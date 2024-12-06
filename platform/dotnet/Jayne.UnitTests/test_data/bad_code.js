cl ass Player extends SingleUserRemoteObject {
    constructor() {
        super();
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