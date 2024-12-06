import React, {Component} from 'react';
import {Link} from 'react-router-dom';

const ExerciseItem = props => (
    <tr>
        <td>{props.exercise.user.username}</td>
        <td>{props.exercise.description}</td>
        <td>{props.exercise.duration} minutes</td>
        <td>{props.exercise.date.toString().substring(0, 10)}</td>
        <td>
            <Link to={"/edit/" + props.exercise.primaryKey}>edit</Link> | <a href="#" onClick={async () => {
            await props.deleteExerciseAsync(props.exercise.primaryKey)
        }}>delete</a>
        </td>
    </tr>
)

export default class ExercisesList extends Component {
    constructor(props) {
        super(props);

        this.deleteExerciseAsync = this.deleteExerciseAsync.bind(this)

        this.state = {
            //TODO: Replace null with a service instance since we're going to use it in a few
            // difference places on this page.
            service: null,
            exercises: [],
            interval_handle: null
        };
    }

    async componentDidMount() {
        try {
            const onUpdate = (e) => {
                if (e.deleted) {
                    //When it's been deleted, remove it from the state
                    this.setState(prevstate => ({
                        exercises: prevstate.exercises.filter(ex => ex.primaryKey !== e.target.primaryKey)
                    }));
                    console.log(`Exercise ${e.target.primaryKey} deleted`);
                } else {
                    //When it's been updated, trigger a re-render.
                    this.setState(this.state);
                }
            };

            const onExerciseAdded = async (exerciseAdded) => {
                const exercise = exerciseAdded.exercise;

                //add the exercise to the list of known exercises in the state
                this.setState(prev => ({
                    exercises: [...prev.exercises, exercise]
                }));

                // TODO: subscribe to updates made to the newly added exercise
                // TODO: add an update listener for it as well specifying onUpdate as the last argument.

                console.log(`Exercise ${exercise.primaryKey} added from event handler`);
            }

            // TODO: subscribe to ExerciseAdded messages sent from the state.service service specifying
            //  onExerciseAdded as the last argument.

            // TODO: replace [] with a call to the service to get all exercises in a list.
            const exercises = [];

            for(const exercise of exercises) {
                // TODO: subscribe to updates made to the exercise
                // TODO: add an update listener for it as well specifying onUpdate as the last argument.
            }

            this.setState({
                exercises: exercises
            });
        } catch (error) {
            console.error(error);
        }
    }

    async deleteExerciseAsync(primaryKey) {
        try {
            // TODO: delete the exercise by its primaryKey by calling the service
            alert("TODO");
        } catch (error) {
            console.error(error);
        }
    }

    async componentWillUnmount() {
        // TODO: stop listening for changes to existing exercises when we leave this page
        // TODO: stop listening for the ExerciseAdded message when we leave this page

        clearInterval(this.state.intervalHandle);
    }

    exerciseList() {
        return this.state.exercises.map(currentexercise => {
            return <ExerciseItem exercise={currentexercise}
                                 deleteExerciseAsync={this.deleteExerciseAsync}
                                 key={currentexercise.primaryKey}
            />;
        })
    }

    render() {
        return (
            <div>
                <h3>Logged Exercises</h3>
                <table className="table">
                    <thead className="thead-light">
                    <tr>
                        <th>Username</th>
                        <th>Description</th>
                        <th>Duration</th>
                        <th>Date</th>
                        <th>Actions</th>
                    </tr>
                    </thead>
                    <tbody>
                    {this.exerciseList()}
                    </tbody>
                </table>
            </div>
        )
    }
}