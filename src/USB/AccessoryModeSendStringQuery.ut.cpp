/*
*  This file is part of aasdk library project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  aasdk is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  aasdk is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with aasdk. If not, see <http://www.gnu.org/licenses/>.
*/

#include <boost/test/unit_test.hpp>
#include <f1x/aasdk/USB/AccessoryModeSendStringQuery.hpp>
#include <f1x/aasdk/USB/UT/AccessoryModeQueryPromiseHandler.mock.hpp>
#include <f1x/aasdk/USB/UT/USBEndpoint.mock.hpp>
#include <f1x/aasdk/USB/UT/USBWrapper.mock.hpp>

namespace f1x {
namespace aasdk {
namespace usb {
namespace ut {

using ::testing::_;
using ::testing::NotNull;
using ::testing::SaveArg;

class AccessoryModeSendStringQueryUnitTest {
   protected:
    AccessoryModeSendStringQueryUnitTest()
        : usbEndpointMock_(std::make_shared<USBEndpointMock>()), usbEndpoint_(usbEndpointMock_.get(), [](auto*) {}), promise_(IAccessoryModeQuery::Promise::defer(ioService_)) {
        promise_->then(std::bind(&AccessoryModeQueryPromiseHandlerMock::onResolve, &promiseHandlerMock_, std::placeholders::_1),
                       std::bind(&AccessoryModeQueryPromiseHandlerMock::onReject, &promiseHandlerMock_, std::placeholders::_1));
    }

    boost::asio::io_context ioService_;
    USBWrapperMock usbWrapperMock_;
    std::shared_ptr<USBEndpointMock> usbEndpointMock_;
    IUSBEndpoint::Pointer usbEndpoint_;
    AccessoryModeQueryPromiseHandlerMock promiseHandlerMock_;
    IAccessoryModeQuery::Promise::Pointer promise_;

    static constexpr uint32_t USB_TYPE_VENDOR = 0x40;
    static constexpr uint32_t ACC_REQ_SEND_STRING = 52;
};

BOOST_FIXTURE_TEST_CASE(AccessoryModeSendStringQuery_SendString, AccessoryModeSendStringQueryUnitTest) {
    common::DataBuffer buffer;
    IUSBEndpoint::Promise::Pointer usbEndpointPromise;
    EXPECT_CALL(*usbEndpointMock_, controlTransfer(_, _, _)).WillOnce(DoAll(SaveArg<0>(&buffer), SaveArg<2>(&usbEndpointPromise)));

    const std::string expectedQueryString = "aasdkTest";
    EXPECT_CALL(usbWrapperMock_, fillControlSetup(NotNull(), LIBUSB_ENDPOINT_OUT | USB_TYPE_VENDOR, ACC_REQ_SEND_STRING, 0, static_cast<uint16_t>(AccessoryModeSendStringType::MANUFACTURER), expectedQueryString.size() + 1));

    AccessoryModeSendStringQuery::Pointer query(std::make_shared<AccessoryModeSendStringQuery>(ioService_, usbWrapperMock_, usbEndpointMock_, AccessoryModeSendStringType::MANUFACTURER, expectedQueryString));

    query->start(std::move(promise_));
    ioService_.run();
    ioService_.reset();

    BOOST_TEST(buffer.size == expectedQueryString.size() + 1 + 8);
    const std::string actualQueryString(buffer.data + 8, buffer.data + buffer.size - 1);
    BOOST_TEST(actualQueryString == expectedQueryString);

    usbEndpointPromise->resolve(buffer.size);

    EXPECT_CALL(promiseHandlerMock_, onReject(_)).Times(0);
    EXPECT_CALL(promiseHandlerMock_, onResolve(usbEndpoint_));
    ioService_.run();
}

BOOST_FIXTURE_TEST_CASE(AccessoryModeSendStringQuery_TransferError, AccessoryModeSendStringQueryUnitTest) {
    common::DataBuffer buffer;
    IUSBEndpoint::Promise::Pointer usbEndpointPromise;
    EXPECT_CALL(*usbEndpointMock_, controlTransfer(_, _, _)).WillOnce(DoAll(SaveArg<0>(&buffer), SaveArg<2>(&usbEndpointPromise)));

    const std::string expectedQueryString = "aasdkTest";
    EXPECT_CALL(usbWrapperMock_, fillControlSetup(NotNull(), LIBUSB_ENDPOINT_OUT | USB_TYPE_VENDOR, ACC_REQ_SEND_STRING, 0, static_cast<uint16_t>(AccessoryModeSendStringType::MANUFACTURER), expectedQueryString.size() + 1));

    AccessoryModeSendStringQuery::Pointer query(std::make_shared<AccessoryModeSendStringQuery>(ioService_, usbWrapperMock_, usbEndpointMock_, AccessoryModeSendStringType::MANUFACTURER, expectedQueryString));

    query->start(std::move(promise_));
    ioService_.run();
    ioService_.reset();

    const error::Error transferError(error::ErrorCode::USB_TRANSFER, LIBUSB_TRANSFER_ERROR);
    usbEndpointPromise->reject(transferError);

    EXPECT_CALL(promiseHandlerMock_, onReject(transferError));
    EXPECT_CALL(promiseHandlerMock_, onResolve(_)).Times(0);
    ioService_.run();
}

BOOST_FIXTURE_TEST_CASE(AccessoryModeSendStringQuery_RejectWhenInProgress, AccessoryModeSendStringQueryUnitTest) {
    common::DataBuffer buffer;
    IUSBEndpoint::Promise::Pointer usbEndpointPromise;
    EXPECT_CALL(*usbEndpointMock_, controlTransfer(_, _, _)).WillOnce(DoAll(SaveArg<0>(&buffer), SaveArg<2>(&usbEndpointPromise)));

    const std::string expectedQueryString = "aasdkTest";
    EXPECT_CALL(usbWrapperMock_, fillControlSetup(NotNull(), LIBUSB_ENDPOINT_OUT | USB_TYPE_VENDOR, ACC_REQ_SEND_STRING, 0, static_cast<uint16_t>(AccessoryModeSendStringType::MANUFACTURER), expectedQueryString.size() + 1));

    AccessoryModeSendStringQuery::Pointer query(std::make_shared<AccessoryModeSendStringQuery>(ioService_, usbWrapperMock_, usbEndpointMock_, AccessoryModeSendStringType::MANUFACTURER, expectedQueryString));

    query->start(std::move(promise_));
    ioService_.run();
    ioService_.reset();

    AccessoryModeQueryPromiseHandlerMock secondPromiseHandlerMock;
    auto secondPromise = IAccessoryModeQuery::Promise::defer(ioService_);
    secondPromise->then(std::bind(&AccessoryModeQueryPromiseHandlerMock::onResolve, &secondPromiseHandlerMock, std::placeholders::_1),
                        std::bind(&AccessoryModeQueryPromiseHandlerMock::onReject, &secondPromiseHandlerMock, std::placeholders::_1));

    EXPECT_CALL(secondPromiseHandlerMock, onReject(error::Error(error::ErrorCode::OPERATION_IN_PROGRESS)));
    EXPECT_CALL(secondPromiseHandlerMock, onResolve(_)).Times(0);
    query->start(std::move(secondPromise));

    ioService_.run();
}

}  // namespace ut
}  // namespace usb
}  // namespace aasdk
}  // namespace f1x
