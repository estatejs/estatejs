// @flow
import React from 'react';

// components
import PageTitle from '../../components/PageTitle';
import CodeBlock from "../../components/CodeBlock";
import {Link} from "react-router-dom";

const ExerciseTrackerPage = (): React$Element<React$FragmentType> => {
    return (
        <>
            <PageTitle
                breadCrumbItems={[
                    { label: 'Examples', path: '/examples' },
                    { label: 'Exercise Tracker (React)', path: '/examples/exercise-tracker', active: true }
                ]}
                title={'Quick Start: Exercise Tracker (React)'}
            />
            This is a fully functional real-time React-based browser application that demonstrates all of Estate's features.
            The following commands will work on Windows, Linux, and macOS.
            <p/>
            <h3>Prerequisites</h3>
            <ol>
                <li>You must have <a href={'https://nodejs.org/en/download/'}>Node.js</a> 14+ installed to use the <code>estate</code> command.</li>
            </ol>
            <h3>Setup a new project</h3>
            <ol>
                <li>Get the <code>estate</code> command by installing the npm package:<CodeBlock language={`bash`} code={`npm install -g estate-tools`}/></li>
                <li>Create a new project named <code>exercise-tracker</code>:<CodeBlock language={`bash`} code={`estate init exercise-tracker`}/></li>
                <li>Change to the new project's directory:<CodeBlock language={`bash`} code={`cd exercise-tracker`}/></li>
                <li>Initialize the worker folder:<CodeBlock language={`bash`} code={`estate worker init`}/></li>
                <li>Deploy the worker backend code to Estate on the internet:<CodeBlock language={`bash`} code={`estate worker boot`}/></li>
                <li>Generate client code to talk the worker:<CodeBlock language={`bash`} code={`estate worker connect`}/></li>
                When asked what package manager to use, accept the default choice (<code>npm</code>).<br/><br/>
                <li>Change to the web site directory:<CodeBlock language={`bash`} code={`cd client`}/></li>
                <li>Run it:<CodeBlock language={`bash`} code={`npm start`}/></li>
            </ol>
            <h3>Take it for a spin</h3>
            <ol>
                <li>Duplicate the browser tab that <code>npm start</code> opened a few times so you have more than one client running at once.</li>
                <li>Add a user and then add an exercise.</li>
                <li>Edit an existing exercise.</li>
                <li>Delete an exercise.</li>
                <li>Consider the key takeaways:
                    <ul>
                        <li><em>All instances of the browser client reflect all changes made in other clients without having to refresh.</em></li>
                        <li><em>All exercise and user changes will persist after restart because their data is stored in the cloud, not the browser.</em></li>
                        <li><em>This was accomplished with only <a href={'https://github.com/estatejs/estate/examples/blob/main/exercise-tracker/worker/index.ts'}> <b>105 lines</b></a> of backend code.</em></li>
                    </ul>
                </li>
            </ol>
        </>
    );
};

export default ExerciseTrackerPage;
