require 'test_helper'

class PbbsSubrunsControllerTest < ActionController::TestCase
  setup do
    @pbbs_subrun = pbbs_subruns(:one)
  end

  test "should get index" do
    get :index
    assert_response :success
    assert_not_nil assigns(:pbbs_subruns)
  end

  test "should get new" do
    get :new
    assert_response :success
  end

  test "should create pbbs_subrun" do
    assert_difference('PbbsSubrun.count') do
      post :create, :pbbs_subrun => @pbbs_subrun.attributes
    end

    assert_redirected_to pbbs_subrun_path(assigns(:pbbs_subrun))
  end

  test "should show pbbs_subrun" do
    get :show, :id => @pbbs_subrun.to_param
    assert_response :success
  end

  test "should get edit" do
    get :edit, :id => @pbbs_subrun.to_param
    assert_response :success
  end

  test "should update pbbs_subrun" do
    put :update, :id => @pbbs_subrun.to_param, :pbbs_subrun => @pbbs_subrun.attributes
    assert_redirected_to pbbs_subrun_path(assigns(:pbbs_subrun))
  end

  test "should destroy pbbs_subrun" do
    assert_difference('PbbsSubrun.count', -1) do
      delete :destroy, :id => @pbbs_subrun.to_param
    end

    assert_redirected_to pbbs_subruns_path
  end
end
