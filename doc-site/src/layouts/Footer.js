// @flow
import React from 'react';
import { Row, Col } from 'react-bootstrap';

const Footer = (): React$Element<any> => {
    const currentYear = new Date().getFullYear();
    return (
        <React.Fragment>
            <footer className="footer">
                <div className="container-fluid">
                    <Row>
                        <Col md={6}>© Warpdrive Technologies, Inc., {currentYear}</Col>

                        <Col md={6}>
                            <div className="text-md-end footer-links d-none d-md-block">
                                {/*<a href="/about">About</a>*/}
                                <a href="/contact">Contact Us</a>
                            </div>
                        </Col>
                    </Row>
                </div>
            </footer>
        </React.Fragment>
    );
};

export default Footer;
