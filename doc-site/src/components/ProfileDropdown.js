// @flow
import React, { useState } from 'react';
import { Link } from 'react-router-dom';
import { Dropdown } from 'react-bootstrap';

type ProfileMenuItem = {
    label: string,
    icon: string,
    redirectTo: string,
};

type ProfileDropdownProps = {
    menuItems: Array<ProfileMenuItem>,
    username: string,
    userTitle?: string,
    userInitials: string
};

type ProfileDropdownState = {
    dropdownOpen?: boolean,
};

const ProfileDropdown = (props: ProfileDropdownProps, state: ProfileDropdownState): React$Element<any> => {
    const [dropdownOpen, setDropdownOpen] = useState(false);

    /*
     * toggle profile-dropdown
     */
    const toggleDropdown = () => {
        setDropdownOpen(!dropdownOpen);
    };

    return (
        <Dropdown show={dropdownOpen} onToggle={toggleDropdown}>
            <Dropdown.Toggle
                variant="link"
                id="dropdown-profile"
                as={Link}
                to="#"
                onClick={toggleDropdown}
                className="nav-link dropdown-toggle nav-user arrow-none me-0">
                <span className="account-user-avatar">
                    <div className="nav-link--user__initials"> {props.userInitials} </div>
                </span>
                <span>
                    <span className="account-user-name">{props.username}</span>
                    <span className="account-position">{props.userTitle}</span>
                </span>
            </Dropdown.Toggle>
            <Dropdown.Menu className="dropdown-menu dropdown-menu-end dropdown-menu-animated topbar-dropdown-menu profile-dropdown">
                <div onClick={toggleDropdown}>
                    <div className="dropdown-header noti-title">
                        <h6 className="text-overflow m-0">Welcome !</h6>
                    </div>
                    {props.menuItems.map((item, i) => {
                        return (
                            <Link to={item.redirectTo} className="dropdown-item notify-item" key={i + '-profile-menu'}>
                                <i className={`${item.icon} me-1`}></i>
                                <span>{item.label}</span>
                            </Link>
                        );
                    })}
                </div>
            </Dropdown.Menu>
        </Dropdown>
    );
};

export default ProfileDropdown;
