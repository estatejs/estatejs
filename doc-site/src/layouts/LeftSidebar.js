// @flow
import React, { useEffect, useRef } from 'react';
import { Link } from 'react-router-dom';
import SimpleBar from 'simplebar-react';
import classNames from 'classnames';

import { getMenuItems } from '../helpers/menu';

// components
import AppMenu from './Menu';

import logoDark from '../assets/images/logo-dark.svg';
import logoDarkSm from '../assets/images/logo-sm-dark.svg';
import helpBoxImage from '../assets/images/help-icon.svg';
import profileImg from '../assets/images/users/avatar-1.jpg';

type SideBarContentProps = {
    hideUserProfile: boolean,
};

/* sidebar content */
const SideBarContent = ({ hideUserProfile }: SideBarContentProps) => {
    return (
        <>
            {!hideUserProfile && (
                <div className="leftbar-user">
                    <Link to="/">
                        <img src={profileImg} alt="" height="42" className="rounded-circle shadow-sm" />
                        <span className="leftbar-user-name">Dominic Keller</span>
                    </Link>
                </div>
            )}

            <AppMenu menuItems={getMenuItems()} />
        </>
    );
};

type LeftSidebarProps = {
    hideLogo: boolean,
    hideUserProfile: boolean,
    isLight: boolean,
    isCondensed: boolean,
};

const LeftSidebar = ({ isCondensed, isLight, hideLogo, hideUserProfile }: LeftSidebarProps): React$Element<any> => {
    const menuNodeRef: any = useRef(null);

    /**
     * Handle the click anywhere in doc
     */
    const handleOtherClick = (e: any) => {
        if (menuNodeRef && menuNodeRef.current && menuNodeRef.current.contains(e.target)) return;
        // else hide the menubar
        if (document.body) {
            document.body.classList.remove('sidebar-enable');
        }
    };

    useEffect(() => {
        document.addEventListener('mousedown', handleOtherClick, false);

        return () => {
            document.removeEventListener('mousedown', handleOtherClick, false);
        };
    }, []);

    return (
        <React.Fragment>
            <div className="leftside-menu" ref={menuNodeRef}>
                {!hideLogo && (
                    <React.Fragment>
                        <Link to="/" className="logo text-center logo-light">
                            <span className="logo-lg">
                                <img src={logoDark} alt="logo" height="42" />
                            </span>
                            <span className="logo-sm">
                                <img src={logoDarkSm} alt="logo" height="10" />
                            </span>
                        </Link>
                        {/*<Link to="/" className="logo text-center logo-dark">*/}
                        {/*    <span className="logo-lg">*/}
                        {/*        <img src={logoDark} alt="logo" height="42" />*/}
                        {/*    </span>*/}
                        {/*    <span className="logo-sm">*/}
                        {/*        <img src={logoDarkSm} alt="logo" height="42" />*/}
                        {/*    </span>*/}
                        {/*</Link>*/}
                    </React.Fragment>
                )}

                {!isCondensed && (
                    <SimpleBar style={{ maxHeight: '100%' }} timeout={500} scrollbarMaxSize={320}>
                        <SideBarContent
                            menuClickHandler={() => {}}
                            isLight={isLight}
                            hideUserProfile={hideUserProfile}
                        />
                    </SimpleBar>
                )}
                {isCondensed && <SideBarContent isLight={isLight} hideUserProfile={hideUserProfile} />}
            </div>
        </React.Fragment>
    );
};

LeftSidebar.defaultProps = {
    hideLogo: false,
    hideUserProfile: false,
    isLight: false,
    isCondensed: false,
};

export default LeftSidebar;
