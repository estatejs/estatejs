// @flow
import React from 'react';

// components
import PageTitle from '../../components/PageTitle';

const MyAccountPage = (): React$Element<React$FragmentType> => {
    return (
        <>
            <PageTitle
                breadCrumbItems={[
                    { label: 'My Account', path: '/account/my-account', active: true }
                ]}
                title={'My Account'}
            />
        </>
    );
};

export default MyAccountPage;
