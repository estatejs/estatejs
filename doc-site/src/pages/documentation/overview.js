// @flow
import React from 'react';

// components
import PageTitle from '../../components/PageTitle';
import {Link, Redirect} from "react-router-dom";

const OverviewPage = (): React$Element<React$FragmentType> => {
    return (
        <>
            <PageTitle
                breadCrumbItems={[
                    {label: 'Documentation', path: '/documentation'},
                    {label: 'Overview', path: '/documentation/overview', active: true}
                ]}
                title={'Overview'}
            />
            Get ready to experience life without DevOps or boilerplate. Welcome to Estate! <p/>
            Estate is a cloud platform where developers create backends by writing plain TypeScript classes that
            define each piece of the architecture and lets the platform handle the cloud minutia.<p/>
            With no servers to manage nor ‘stacks’ to provision, Estate backends deploy with a single command and
            scale automatically to meet demand.<p/>
            Estate provides everything a developer needs to build fully functional, modern backends.<p/>

            <i>Want a microservice?</i><p/>
            <ul>
                <li>Write a class that extends Service.</li>
                <li>Get an instance in client code, call methods on shared stateful services.</li>
            </ul>

            <i>Want to store and retrieve observable cloud data?</i><p/>
            <ul>
                <li>Write a class that extends Data.</li>
                <li>Store or retrieve instances from client code or backend services.</li>
                <li>Optionally, clients can be updated when other clients or services save data changes.</li>
            </ul>

            <i>Want to notify any number of clients when something happens?</i>
            <ul>
                <li>Write a class that extends Message.</li>
                <li>Send message instances from inside services.</li>
                <li>Write a handler in client code that gets called when the message is received.</li>
            </ul>

            No more REST. No more interpreting HTTP status code arcanery or web service routing nonsense. Consuming a
            Estate backend is trivial because all the client-connection code is generated for you.<p/>
            Spend your time building features instead of boilerplate.<p/>


            Get started now by following the <Link to={"/documentation/quick-start"} >Quick Start</Link> guide.
            <p/>
        </>
    );
};

export default OverviewPage;
