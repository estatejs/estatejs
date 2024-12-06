// @flow
import React from 'react';

// components
import PageTitle from '../components/PageTitle';

const AboutPage = (): React$Element<React$FragmentType> => {
    return (
        <>
            <PageTitle
                breadCrumbItems={[
                    { label: 'About', path: '/about', active: true }
                ]}
                title={'About (Coming Soon!)'}
            />
            <em>This is a stub, more information will be added soon.</em><p/>
        </>
    );
};

export default AboutPage;
