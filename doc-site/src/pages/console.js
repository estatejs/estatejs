// @flow
import React from 'react';

// components
import PageTitle from '../components/PageTitle';

const ConsolePage = (): React$Element<React$FragmentType> => {
    return (
        <>
            <PageTitle
                breadCrumbItems={[
                    { label: 'Console', path: '/console', active: true }
                ]}
                title={'Console (Coming Soon!)'}
            />
            <em>This is where you'll view and manage your deployed Estate data, services, and messages.</em>
        </>
    );
};

export default ConsolePage;
