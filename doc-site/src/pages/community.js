// @flow
import React from 'react';

// components
import PageTitle from '../components/PageTitle';
import {SLACK_INVITE_URL} from "../config";

const CommunityPage = (): React$Element<React$FragmentType> => {
    return (
        <>
            <PageTitle
                breadCrumbItems={[
                    { label: 'Community', path: '/community', active: true }
                ]}
                title={'Community'}
            />
            <h3>Join in!</h3>
            <p/>Please join our <a href={SLACK_INVITE_URL}>Slack community</a>.
            <h3>Found a bug?</h3>
            <p/>If you've found a bug please open an issue on <a href={'https://github.com/estatejs/estate/issues'}>GitHub</a>. If it's urgent please follow up in Slack and we'll be happy to help.
        </>
    );
};

export default CommunityPage;
