import React, {Component} from 'react';
import { Navigate } from 'react-router-dom';

export default class CreateUser extends Component {
    constructor(props) {
        super(props);

        this.onChangeUsername = this.onChangeUsername.bind(this);
        this.onSubmit = this.onSubmit.bind(this);

        this.state = {
            username: '',
            redirect: false
        }
    }

    onChangeUsername(e) {
        this.setState({
            username: e.target.value
        })
    }

    async onSubmit(e) {
        e.preventDefault();

        try
        {
            alert("TODO");

            // TODO: get a service reference and create a user by passing the username from the form.

            this.setState({
             username: '',
             redirect: true
            });
            console.log(`User ${this.state.username} added`);
        }
        catch(error)
        {
            console.error(error);
        }
    }

    render() {
        if(this.state.redirect) {
            return (<Navigate replace to="/create"/>);
        }

        return (
            <div>
                <h3>Create New User</h3>
                <form onSubmit={this.onSubmit}>
                    <div className="form-group">
                        <label>Username: </label>
                        <input type="text"
                               required
                               className="form-control"
                               value={this.state.username}
                               onChange={this.onChangeUsername}
                        />
                    </div>
                    <div className="form-group">
                        <input type="submit" value="Create User" className="btn btn-primary"/>
                    </div>
                </form>
            </div>
        )
    }
}